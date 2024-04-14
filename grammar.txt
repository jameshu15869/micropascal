<compilationUnit> := "program" <identifier> <block> "."

<block> := (<declaration>)? "begin" <statementSequence> "end";

<declaration> := "const" <identifier> (<constantDeclaration> ";" )* | "var" (<variableDeclaration> ";")* | <functionDeclaration> ";" | <procedureDeclaration> ";"

<constantDeclaration> := <identifier> "=" <expression>
<variableDeclaration> := <identifierList> ":" <type>

<identifierList> := <identifier> ("," <identifier>)*

<type> := "real" | "string" | "boolean" | "integer"

<functionDeclaration> := "function" <identifier> (<functionParameters>)? ";" <block>
<functionParameters> := "(" (<formalParameterList>)? ")" ":" <type>

<procedureDeclaration> := "procedure" <identifier> (<procedureParameters>)? ";" <block> // semicolon handled by <declaration>

<procedureParameters> := "(" (<formalParameterList>)? ")"
<formalParameterList> := <formalParameter> ("," <formalParameter>)*
<formalParameter> := "const" <identifierList> ":" <type> | "var" <identifierList> ":" <type>

<statementSequence> := <statement> (";" <statement>)*
<statement> := <identifier> { ":=" <expression> // assignment | ( "(" (<expressionList>)? ")" // function call)}
                | <ifStatement>
                | <whileStatement>

<ifStatement> := "if" <expression> "then" <statementSequence> ("else" <statementSequence>)?

<whileStatement> := "while" <expression> "do" <block> ";"

<expressionList> := <expression> ("," <expression>)*

<expression> := <simpleExpression> (<relation> <simpleExpression>)?

<relation> := "<" | "=" | "<="

<simpleExpression> := <term> (<addOperator> <term>)?

<addOperator> := "+" | "-"

<term> := <factor> (<mulOperator> <factor>)?

<mulOperator> := "*" | "/" | "div"

<factor> := integer | "(" <expression> ")"

