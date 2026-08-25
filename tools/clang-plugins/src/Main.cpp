#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang-tidy/ClangTidyModule.h>
#include <clang-tidy/utils/OptionsUtils.h>
#include <clang-tidy/ClangTidyCheck.h>
#include <clang/Lex/Lexer.h>
#include <string>
#include <vector>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tidy;
using namespace clang::ast_matchers::internal;

namespace uefi {
    namespace {
        // =========================================================================
        // Check 1: Necessary TRACE_FUNCTION() in the start of the function
        // =========================================================================
        class TraceFunctionCheck : public ClangTidyCheck {
        private:
            const std::string rawTargetFiles;
            const std::vector<StringRef> targetFiles;

        public:
            TraceFunctionCheck(StringRef name, ClangTidyContext* context)
                : ClangTidyCheck(name, context), rawTargetFiles(Options.get("TargetFiles", "")),
                  targetFiles(utils::options::parseStringList(rawTargetFiles)) {}
            void storeOptions(ClangTidyOptions::OptionMap& opts) override { Options.store(opts, "TargetFiles", rawTargetFiles); }

            void registerMatchers(MatchFinder* finder) override { // All function definitions
                finder->addMatcher(functionDecl(isDefinition(), isExpansionInMainFile()).bind("func"), this);
            }

            // checks function from files from TargetFiles option to TRACE_CHECK first macro statement
            void check(const MatchFinder::MatchResult& result) override {
                const auto* FD = result.Nodes.getNodeAs<FunctionDecl>("func");
                if (!FD || !FD->hasBody()) return;

                const SourceManager& SM = *result.SourceManager;
                StringRef fullPath = SM.getFilename(FD->getLocation());
                StringRef baseName = llvm::sys::path::filename(fullPath);
                // ---SELECTIVE FILE CHECK---
                bool fileMatches = targetFiles.empty();
                for (StringRef target : targetFiles) {
                    if (baseName == target.trim()) {
                        fileMatches = true;
                        break;
                    }
                }
                if (!fileMatches) return;
                // ----------------------------

                if (FD->getNameAsString() == "DriverEntryPoint") return;

                const auto* body = dyn_cast<CompoundStmt>(FD->getBody());
                if (!body || body->body_empty()) {
                    diag(FD->getLocation(), "Function '%0' is empty and missing TRACE_FUNCTION();") << FD->getNameAsString();
                    return;
                }

                const Stmt* firstStmt = *body->body_begin();
                const LangOptions langOpts = result.Context->getLangOpts();
                const SourceLocation loc = firstStmt->getBeginLoc();

                bool isTraceMacro = false;

                // unwinding the chain of macros
                SourceLocation L = loc;
                while (L.isMacroID()) {
                    StringRef macroName = Lexer::getImmediateMacroName(L, SM, langOpts);
                    if (macroName == "TRACE_FUNCTION") {
                        isTraceMacro = true;
                        break;
                    }
                    L = SM.getImmediateMacroCallerLoc(L);
                }

                if (!isTraceMacro)
                    diag(SM.getExpansionLoc(loc), "Function '%0' must start with TRACE_FUNCTION();") << FD->getNameAsString();
            }
        };

        // =========================================================================
        // Check 2: Blocking standard UEFI allocators
        // =========================================================================
        class BannedAllocatorCheck : public ClangTidyCheck {
        public:
            BannedAllocatorCheck(StringRef name, ClangTidyContext* context) : ClangTidyCheck(name, context) {}

            void registerMatchers(MatchFinder* finder) override {
                auto bannedNames = hasAnyName("AllocatePool", "AllocatePages", "FreePool", "FreePages");

                // catching func calls:
                // 1. declRefExpr() — catches direct calls like AllocatePool() (from Library)
                // 2. memberExpr()  — catches calls from the table like gBS->AllocatePool()
                auto matcher = callExpr(callee(namedDecl(bannedNames))).bind("bad_alloc");

                finder->addMatcher(matcher, this);
            }

            void check(const MatchFinder::MatchResult& result) override {
                const auto* matchedCall = result.Nodes.getNodeAs<CallExpr>("bad_alloc");
                if (!matchedCall) return;

                const SourceManager& SM = *result.SourceManager;
                StringRef fullPath = SM.getFilename(SM.getFileLoc(matchedCall->getBeginLoc()));
                StringRef fileName = llvm::sys::path::filename(fullPath);
                if (fileName == "Allocator.c") return; // Allow raw allocation APIs inside the custom allocator implementation

                diag(matchedCall->getBeginLoc(), "Using default allocators is blocked (AllocatePool, FreePool, etc.). "
                                                 "Use custom project allocator.");
            }
        };

        // =========================================================================
        // Check 3: Warning for unchecked UEFI types that works for function pointers as well unlike build-in clang
        // =========================================================================
        // reference:
        // https://github.com/llvm/llvm-project/blob/main/clang-tools-extra/clang-tidy/bugprone/UnusedReturnValueCheck.cpp
        constexpr std::initializer_list<OverloadedOperatorKind> assignmentOverloadedOperatorKinds = {
            OO_Equal,    OO_PlusEqual, OO_MinusEqual,    OO_StarEqual,           OO_SlashEqual, OO_PercentEqual, OO_CaretEqual,
            OO_AmpEqual, OO_PipeEqual, OO_LessLessEqual, OO_GreaterGreaterEqual, OO_PlusPlus,   OO_MinusMinus};

        AST_MATCHER(FunctionDecl, isAssignmentOverloadedOperator) {
            return llvm::is_contained(assignmentOverloadedOperatorKinds, Node.getOverloadedOperator());
        }
        AST_MATCHER(Expr, isLastStmtInStmtExpr) {
            ASTContext& ctx = Finder->getASTContext();
            auto parents = ctx.getParents(Node);
            if (parents.empty()) return false;

            // Check if the parent is a CompoundStmt
            const auto* parentBlock = parents[0].get<CompoundStmt>();
            if (!parentBlock || parentBlock->body_empty()) return false;

            if (parentBlock->body_back() != &Node) return false; // Check if the current expression is exactly the last statement

            // Check if the CompoundStmt is the body of a GNU StmtExpr ({...}) - parent block is {...}
            auto grandParents = ctx.getParents(*parentBlock);
            if (grandParents.empty()) return false;

            return grandParents[0].get<StmtExpr>() != nullptr; // grandParents[0] == (...)
        }

        class UncheckedStatusCheck : public ClangTidyCheck {
        public:
            UncheckedStatusCheck(StringRef Name, ClangTidyContext* Context) : ClangTidyCheck(Name, Context) {}

            // doesn't match void casts, works with gnu statements unlike clang implementation
            void registerMatchers(MatchFinder* finder) override {
                // 1. Identify specific return types
                // removed returns
                auto matchesSpecificType = qualType(
                    hasDeclaration(namedDecl(hasAnyName("EFI_STATUS", "RETURN_STATUS", "::EFI_STATUS", "::RETURN_STATUS"))));

                // 2. Identify a direct call that returns the status
                // changed logic for matching struct calls as well like gST->ConOut->ClearScreen(gST->ConOut)
                auto matchedDirectCallExpr =
                    expr(callExpr(hasType(matchesSpecificType), // Match functions returning your specific type
                                                                // removed unless(returns(voidType()))
                                  unless(callee(functionDecl(
                                      isAssignmentOverloadedOperator()))))) // Don't match copy or move assignment operator.
                        .bind("ignored_call");

                const auto checkCastToVoid = castExpr(unless(hasCastKind(CK_ToVoid))); // removed choice for checking case to void

                // 3. Match the call or cast to target types except cast to void(combined matchedDirectCallExpr and
                // checkCastToVoid) removed unless(cxxFunctionalCastExpr())
                const auto matchedCallExpr = expr(
                    anyOf(matchedDirectCallExpr, explicitCastExpr(checkCastToVoid, hasSourceExpression(matchedDirectCallExpr))));

                // 4. NEW: Match a GNU StmtExpr where the LAST statement is our matched call
                auto matchedLastInStmtExpr =
                    stmtExpr(has(compoundStmt(hasAnySubstatement(expr(matchedCallExpr, isLastStmtInStmtExpr())))));

                // 5. NEW: Combined matchedCallExpr and matchedLastInStmtExpr
                // An expression is discarded if:
                // A) It is a standard matched call, AND it is NOT the last stmt of a GNU block.
                // B) It IS the last stmt of a GNU block, which implies the whole GNU block is discarded.
                auto discardedExpr = expr(anyOf(expr(matchedCallExpr, unless(isLastStmtInStmtExpr())), matchedLastInStmtExpr));

                auto unusedInCompoundStmt =
                    compoundStmt(forEach(discardedExpr)); // removed unless(hasParent(stmtExpr())) cuz added support of gnu blocks

                auto unusedInForStmt =
                    forStmt(eachOf(hasLoopInit(discardedExpr), hasIncrement(discardedExpr), hasBody(discardedExpr)));
                // below is in case of absent braces for spec statements
                auto unusedInIfStmt = ifStmt(eachOf(hasThen(discardedExpr), hasElse(discardedExpr)));
                auto unusedInWhileStmt = whileStmt(hasBody(discardedExpr));
                auto unusedInDoStmt = doStmt(hasBody(discardedExpr));
                auto unusedInRangeForStmt = cxxForRangeStmt(hasBody(discardedExpr));
                auto unusedInCaseStmt = switchCase(forEach(discardedExpr));

                finder->addMatcher(stmt(anyOf(unusedInCompoundStmt, unusedInIfStmt, unusedInWhileStmt, unusedInDoStmt,
                                              unusedInForStmt, unusedInRangeForStmt, unusedInCaseStmt)),
                                   this);
            }

            void check(const MatchFinder::MatchResult& Result) override {
                const auto* Call = Result.Nodes.getNodeAs<CallExpr>("ignored_call");
                if (!Call) return;

                diag(Call->getBeginLoc(),
                     "Return value of type 'EFI_STATUS' must not be ignored (wrap in CHECK_FOR_ERROR or handle it).");
            }
        };

        // MODULE REGISTRATION
        class UefiModule : public ClangTidyModule {
        public:
            void addCheckFactories(ClangTidyCheckFactories& checkFactories) override {
                checkFactories.registerCheck<TraceFunctionCheck>("uefi-trace-function");
                checkFactories.registerCheck<BannedAllocatorCheck>("uefi-banned-allocator");
                checkFactories.registerCheck<UncheckedStatusCheck>("uefi-unchecked-status");
            }
        };
    } // unnamed namespace
} // namespace uefi

// MODULE REGISTRATION in global registry Clang-Tidy
namespace clang::tidy {
    static ClangTidyModuleRegistry::Add<uefi::UefiModule>
        X("uefi-module", "Adds UEFI specific checks (trace macro & memory allocation restrictions).");
    static volatile int uefiModuleAnchorSource = 0;
} // namespace clang::tidy