

void UnusedReturnValueCheck::registerMatchers(MatchFinder* Finder) {
    auto MatchedDirectCallExpr =
        expr(callExpr(callee(functionDecl(
                          // Don't match copy or move assignment operator.
                          unless(isAssignmentOverloadedOperator()),
                          // Don't match void overloads of checked functions.
                          unless(returns(voidType())),
                          anyOf(isInstantiatedFrom(matchers::matchesAnyListedRegexName(CheckedFunctions)),
                                returns(hasCanonicalType(
                                    hasDeclaration(namedDecl(matchers::matchesAnyListedRegexName(CheckedReturnTypes)))))))))
                 .bind("match"));

    const auto CheckCastToVoid = AllowCastToVoid ? castExpr(unless(hasCastKind(CK_ToVoid))) : castExpr();
    const auto MatchedCallExpr =
        expr(anyOf(MatchedDirectCallExpr, explicitCastExpr(unless(cxxFunctionalCastExpr()), CheckCastToVoid,
                                                           hasSourceExpression(MatchedDirectCallExpr))));

    auto UnusedInCompoundStmt = compoundStmt(forEach(MatchedCallExpr),
                                             // The checker can't currently differentiate between the
                                             // return statement and other statements inside GNU statement
                                             // expressions, so disable the checker inside them to avoid
                                             // false positives.
                                             unless(hasParent(stmtExpr())));
    auto UnusedInIfStmt = ifStmt(eachOf(hasThen(MatchedCallExpr), hasElse(MatchedCallExpr)));
    auto UnusedInWhileStmt = whileStmt(hasBody(MatchedCallExpr));
    auto UnusedInDoStmt = doStmt(hasBody(MatchedCallExpr));
    auto UnusedInForStmt = forStmt(eachOf(hasLoopInit(MatchedCallExpr), hasIncrement(MatchedCallExpr), hasBody(MatchedCallExpr)));
    auto UnusedInRangeForStmt = cxxForRangeStmt(hasBody(MatchedCallExpr));
    auto UnusedInCaseStmt = switchCase(forEach(MatchedCallExpr));

    Finder->addMatcher(stmt(anyOf(UnusedInCompoundStmt, UnusedInIfStmt, UnusedInWhileStmt, UnusedInDoStmt, UnusedInForStmt,
                                  UnusedInRangeForStmt, UnusedInCaseStmt)),
                       this);
}