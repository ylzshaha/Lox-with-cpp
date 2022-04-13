#ifndef __TOKEN_TYPE_
#define __TOKEN_TYPE_
#define DEBUG


using namespace std;
enum TokenType {
  // Single-character tokens.
  LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
  COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR, LEFT_BRACKET, RIGHT_BRACKET,

  // One or two character tokens.
  BANG, BANG_EQUAL,
  EQUAL, EQUAL_EQUAL,
  GREATER, GREATER_EQUAL,
  LESS, LESS_EQUAL,

  // Literals.
  IDENTIFIER, STRING, NUMBER,

  // Keywords.
  AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR,
  PRINT, RETURN, SUPER, THIS, TRUE, VAR, WHILE,
  EoF
};


#define TOKEN_GROUP(KEYWORD_TOKEN, ODINARY_TOKEN)                       \
  ODINARY_TOKEN("LEFT_PAREN", TokenType::LEFT_PAREN)                    \
  ODINARY_TOKEN("RIGHT_PAREN", TokenType::RIGHT_PAREN)                  \
  ODINARY_TOKEN("LEFT_BRACE", TokenType::LEFT_BRACE)                    \
  ODINARY_TOKEN("RIGHT_BRACE", TokenType::RIGHT_BRACE)                  \
  ODINARY_TOKEN("COMMA", TokenType::COMMA)                              \
  ODINARY_TOKEN("DOT", TokenType::DOT)                                  \
  ODINARY_TOKEN("MINUS", TokenType::MINUS)                              \
  ODINARY_TOKEN("PLUS", TokenType::PLUS)                                \
  ODINARY_TOKEN("SEMICOLON", TokenType::SEMICOLON)                      \
  ODINARY_TOKEN("SLASH", TokenType::SLASH)                              \
  ODINARY_TOKEN("STAR", TokenType::STAR)                                \
  ODINARY_TOKEN("LEFT_BRACKET", TokenType::LEFT_BRACKET)                \
  ODINARY_TOKEN("RIGHT_BRACKET", TokenType::RIGHT_BRACKET)              \
  ODINARY_TOKEN("BANG", TokenType::BANG)                                \
  ODINARY_TOKEN("BANG_EQUAL", TokenType::BANG_EQUAL)                    \
  ODINARY_TOKEN("EQUAL", TokenType::EQUAL)                              \
  ODINARY_TOKEN("EQUAL_EQUAL", TokenType::EQUAL_EQUAL)                  \
  ODINARY_TOKEN("GREATER", TokenType::GREATER)                          \
  ODINARY_TOKEN("GREATER_EQUAL", TokenType::GREATER_EQUAL)              \
  ODINARY_TOKEN("LESS", TokenType::LESS)                                \
  ODINARY_TOKEN("LESS_EQUAL", TokenType::LESS_EQUAL)                    \
  ODINARY_TOKEN("IDENTIFIER", TokenType::IDENTIFIER)                    \
  ODINARY_TOKEN("STRING", TokenType::STRING)                            \
  ODINARY_TOKEN("NUMBER", TokenType::NUMBER)                            \
  KEYWORD_TOKEN("and", TokenType::AND)                                  \
  KEYWORD_TOKEN("class", TokenType::CLASS)                              \
  KEYWORD_TOKEN("else", TokenType::ELSE)                                \
  KEYWORD_TOKEN("false", TokenType::FALSE)                              \
  KEYWORD_TOKEN("fun", TokenType::FUN)                                  \
  KEYWORD_TOKEN("for", TokenType::FOR)                                  \
  KEYWORD_TOKEN("if", TokenType::IF)                                    \
  KEYWORD_TOKEN("nil", TokenType::NIL)                                  \
  KEYWORD_TOKEN("or", TokenType::OR)                                    \
  KEYWORD_TOKEN("print", TokenType::PRINT)                              \
  KEYWORD_TOKEN("return", TokenType::RETURN)                            \
  KEYWORD_TOKEN("super", TokenType::SUPER)                              \
  KEYWORD_TOKEN("this", TokenType::THIS)                                \
  KEYWORD_TOKEN("true", TokenType::TRUE)                                \
  KEYWORD_TOKEN("var", TokenType::VAR)                                  \
  KEYWORD_TOKEN("while", TokenType::WHILE)                               

#endif
