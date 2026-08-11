#include "clang/AST/ASTConsumer.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Lexer.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

static llvm::cl::OptionCategory TraceCheckerCategory("trace-checker options");

class TraceMatchHandler : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult& Result) override {
        const auto* FD = Result.Nodes.getNodeAs<FunctionDecl>("func");
        if (!FD || !FD->hasBody()) return;

        if (FD->getNameAsString() == "DriverEntryPoint") return;

        const auto* Body = dyn_cast<CompoundStmt>(FD->getBody());
        if (!Body || Body->body_empty()) {
            reportError(Result, FD->getLocation(),
                        "Function '" + FD->getNameAsString() + "' is empty and missing TRACE_FUNCTION();");
            return;
        }

        const Stmt* FirstStmt = *Body->body_begin();
        SourceManager& SM = *Result.SourceManager;
        LangOptions LangOpts = Result.Context->getLangOpts();
        SourceLocation Loc = FirstStmt->getBeginLoc();

        bool isTraceMacro = false;

        // 1. Раскручиваем ВСЮ цепочку вложенных макросов (DEBUG -> TRACE_FUNCTION)
        SourceLocation L = Loc;
        while (L.isMacroID()) {
            StringRef MacroName = Lexer::getImmediateMacroName(L, SM, LangOpts);
            if (MacroName == "TRACE_FUNCTION") {
                isTraceMacro = true;
                break;
            }
            // Поднимаемся выше к родительскому макросу
            L = SM.getImmediateMacroCallerLoc(L);
        }

        // 2. Запасная проверка по тексту исходного кода
        if (!isTraceMacro) {
            SourceLocation ExpLoc = SM.getExpansionLoc(Loc);
            StringRef StmtText = Lexer::getSourceText(CharSourceRange::getTokenRange(ExpLoc), SM, LangOpts);
            if (StmtText.starts_with("TRACE_FUNCTION")) isTraceMacro = true;
        }

        // Если это не TRACE_FUNCTION — выводим ошибку
        if (!isTraceMacro) {
            reportError(Result, SM.getExpansionLoc(Loc),
                        "Function '" + FD->getNameAsString() + "' must start with TRACE_FUNCTION();");
        }
    }

private:
    void reportError(const MatchFinder::MatchResult& Result, SourceLocation Loc, const std::string& Msg) {
        DiagnosticsEngine& Diag = Result.Context->getDiagnostics();
        unsigned DiagID = Diag.getCustomDiagID(DiagnosticsEngine::Error, "%0");
        Diag.Report(Loc, DiagID) << Msg;
    }
};

int main(int argc, const char** argv) {
    auto ExpectedParser = CommonOptionsParser::create(argc, argv, TraceCheckerCategory);
    if (!ExpectedParser) {
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }

    CommonOptionsParser& OptionsParser = ExpectedParser.get();
    ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());

    TraceMatchHandler Handler;
    MatchFinder Finder;

    Finder.addMatcher(functionDecl(isDefinition(), isExpansionInMainFile()).bind("func"), &Handler);

    return Tool.run(newFrontendActionFactory(&Finder).get());
}