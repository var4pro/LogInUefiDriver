#include <clang-tidy/ClangTidyCheck.h>
#include <clang-tidy/ClangTidyModule.h>
#include <clang-tidy/utils/OptionsUtils.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Lex/Lexer.h>
#include <string>
#include <vector>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tidy;

namespace uefi {
    namespace {
        // =========================================================================
        // Check 1: Necessary TRACE_FUNCTION() in the start of the function
        // =========================================================================
        class TraceFunctionCheck : public ClangTidyCheck {
        private:
            const std::string RawTargetFiles;
            const std::vector<StringRef> TargetFiles;

        public:
            TraceFunctionCheck(StringRef name, ClangTidyContext* context)
                : ClangTidyCheck(name, context), RawTargetFiles(Options.get("TargetFiles", "")),
                  TargetFiles(utils::options::parseStringList(RawTargetFiles)) {}
            void storeOptions(ClangTidyOptions::OptionMap& Opts) override { Options.store(Opts, "TargetFiles", RawTargetFiles); }

            void registerMatchers(MatchFinder* finder) override { // All function definitions
                finder->addMatcher(functionDecl(isDefinition(), isExpansionInMainFile()).bind("func"), this);
            }

            void check(const MatchFinder::MatchResult& result) override {
                const auto* FD = result.Nodes.getNodeAs<FunctionDecl>("func");
                if (!FD || !FD->hasBody()) return;

                const SourceManager& SM = *result.SourceManager;
                StringRef fullPath = SM.getFilename(FD->getLocation());
                StringRef baseName = llvm::sys::path::filename(fullPath);
                // ---SELECTIVE FILE CHECK---
                bool fileMatches = TargetFiles.empty();
                for (StringRef target : TargetFiles) {
                    if (baseName == target.trim()) {
                        fileMatches = true;
                        break;
                    }
                }
                if (!fileMatches) return;
                // ----------------------------

                if (FD->getNameAsString() == "DriverEntryPoint") return;

                const auto* body = dyn_cast<CompoundStmt>(FD->getBody());
                if (!body || body->body_empty()) { // TODO: need to check this shit and understand later
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

                // spare check
                if (!isTraceMacro) {
                    // llvm::errs() << "I'M INSIDE SPARE CHECK";
                    const SourceLocation expLoc = SM.getExpansionLoc(loc);
                    StringRef stmtText = Lexer::getSourceText(CharSourceRange::getTokenRange(expLoc), SM, langOpts);
                    if (stmtText == ("TRACE_FUNCTION")) isTraceMacro = true;
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
                auto bannedNames =
                    hasAnyName("AllocatePool", "AllocateZeroPool", "AllocateCopyPool", "AllocatePages", "FreePool", "FreePages");

                // catching func calls:
                // 1. declRefExpr() — catches direct calls like AllocatePool() (from Library)
                // 2. memberExpr()  — catches calls from the table like gBS->AllocatePool()
                auto matcher =
                    callExpr(callee(expr(anyOf(declRefExpr(to(functionDecl(bannedNames))), memberExpr(member(bannedNames))))))
                        .bind("bad_alloc");

                finder->addMatcher(matcher, this);
            }

            void check(const MatchFinder::MatchResult& Result) override {
                const auto* matchedCall = Result.Nodes.getNodeAs<CallExpr>("bad_alloc");
                if (!matchedCall) return;

                diag(matchedCall->getBeginLoc(), "Using default allocators is blocked (AllocatePool, FreePool, etc.). "
                                                 "Use custom project allocator.");
            }
        };

        // MODULE REGISTRATION
        class UefiModule : public ClangTidyModule {
        public:
            void addCheckFactories(ClangTidyCheckFactories& checkFactories) override {
                checkFactories.registerCheck<TraceFunctionCheck>("uefi-trace-function");
                checkFactories.registerCheck<BannedAllocatorCheck>("uefi-banned-allocators");
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