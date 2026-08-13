#include "clang-tidy/ClangTidyCheck.h"
#include "clang-tidy/ClangTidyModule.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tidy;

namespace uefi {

    // =========================================================================
    // ЧЕК 1: Проверка на обязательный макрос TRACE_FUNCTION() в начале функции
    // =========================================================================
    class TraceFunctionCheck : public ClangTidyCheck {
    public:
        TraceFunctionCheck(StringRef Name, ClangTidyContext* Context) : ClangTidyCheck(Name, Context) {}

        void registerMatchers(MatchFinder* Finder) override {
            // Ищем все определения функций в главном файле
            Finder->addMatcher(functionDecl(isDefinition(), isExpansionInMainFile()).bind("func"), this);
        }

        void check(const MatchFinder::MatchResult& Result) override {
            const auto* FD = Result.Nodes.getNodeAs<FunctionDecl>("func");
            if (!FD || !FD->hasBody()) return;

            // --- НОВАЯ ФИШКА: ФИЛЬТР ПО ФАЙЛУ ---
            SourceManager& SM = *Result.SourceManager;
            StringRef FileName = SM.getFilename(FD->getLocation());

            // Если это не Main.c, просто выходим (ничего не проверяем)
            if (!FileName.ends_with("Main.c")) return;
            // ------------------------------------

            if (FD->getNameAsString() == "DriverEntryPoint") return;

            const auto* Body = dyn_cast<CompoundStmt>(FD->getBody());
            if (!Body || Body->body_empty()) {
                diag(FD->getLocation(), "Function '%0' is empty and missing TRACE_FUNCTION();") << FD->getNameAsString();
                return;
            }

            const Stmt* FirstStmt = *Body->body_begin();
            LangOptions LangOpts = Result.Context->getLangOpts();
            SourceLocation Loc = FirstStmt->getBeginLoc();

            bool isTraceMacro = false;

            // Раскручиваем цепочку макросов
            SourceLocation L = Loc;
            while (L.isMacroID()) {
                StringRef MacroName = Lexer::getImmediateMacroName(L, SM, LangOpts);
                if (MacroName == "TRACE_FUNCTION") {
                    isTraceMacro = true;
                    break;
                }
                L = SM.getImmediateMacroCallerLoc(L);
            }

            // Запасная проверка по тексту
            if (!isTraceMacro) {
                SourceLocation ExpLoc = SM.getExpansionLoc(Loc);
                StringRef StmtText = Lexer::getSourceText(CharSourceRange::getTokenRange(ExpLoc), SM, LangOpts);
                if (StmtText.starts_with("TRACE_FUNCTION")) isTraceMacro = true;
            }

            if (!isTraceMacro) {
                // diag() сам поймет, как это выводить (Warning или Error на основе .clang-tidy)
                diag(SM.getExpansionLoc(Loc), "Function '%0' must start with TRACE_FUNCTION();") << FD->getNameAsString();
            }
        }
    };

    // =========================================================================
    // ЧЕК 2: Запрет стандартных UEFI аллокаторов
    // =========================================================================
    class BannedAllocatorCheck : public ClangTidyCheck {
    public:
        BannedAllocatorCheck(StringRef Name, ClangTidyContext* Context) : ClangTidyCheck(Name, Context) {}

        void registerMatchers(MatchFinder* Finder) override {
            // Список запрещенных имен
            auto BannedNames =
                hasAnyName("AllocatePool", "AllocateZeroPool", "AllocateCopyPool", "AllocatePages", "FreePool", "FreePages");

            // Ловим вызовы функций, которые подпадают под эти имена:
            // 1. declRefExpr() — ловит прямые вызовы вроде AllocatePool() (из Library)
            // 2. memberExpr()  — ловит вызовы через таблицу вроде gBS->AllocatePool()
            auto Matcher =
                callExpr(callee(expr(anyOf(declRefExpr(to(functionDecl(BannedNames))), memberExpr(member(BannedNames))))))
                    .bind("bad_alloc");

            Finder->addMatcher(Matcher, this);
        }

        void check(const MatchFinder::MatchResult& Result) override {
            const auto* MatchedCall = Result.Nodes.getNodeAs<CallExpr>("bad_alloc");
            if (!MatchedCall) return;

            diag(MatchedCall->getBeginLoc(), "Использование стандартных аллокаторов (AllocatePool, FreePool и т.д.) запрещено. "
                                             "Используй кастомный аллокатор проекта.");
        }
    };

    // =========================================================================
    // РЕГИСТРАЦИЯ МОДУЛЯ
    // =========================================================================
    class UefiModule : public ClangTidyModule {
    public:
        void addCheckFactories(ClangTidyCheckFactories& CheckFactories) override {
            // Регистрируем чеки. Префикс "uefi-" будет использоваться в .clang-tidy конфиге
            CheckFactories.registerCheck<TraceFunctionCheck>("uefi-trace-function");
            CheckFactories.registerCheck<BannedAllocatorCheck>("uefi-banned-allocators");
        }
    };

} // namespace uefi

// Регистрируем наш модуль в глобальном реестре Clang-Tidy
namespace clang::tidy {
    static ClangTidyModuleRegistry::Add<uefi::UefiModule>
        X("uefi-module", "Adds UEFI specific checks (trace macro & memory allocation restrictions).");
    volatile int UefiModuleAnchorSource = 0;
} // namespace clang::tidy