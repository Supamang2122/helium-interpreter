#include "parser.h"

#define streq(a, b) strcmp(a, b) == 0

int precedence(parser* p, lxtoken* op);
astnode* apply_op(vector* primaries, vector* operators);

lxtoken* peek(parser* p)
{
    return vector_get(&p->tokens, p->position);
}

lxtoken* eat(parser* p)
{
    if (!is_empty(p)) {
        return vector_get(&p->tokens, p->position++);
    } else {
        return NULL;
    }
}

lxtoken* consume(parser* p, lxtype type)
{
    if (!is_empty(p) && peek(p)->type == type) {
        return eat(p);
    } else {
        parsererror(p, "Unexpected token");
        return NULL;
    }
}

boolean consume_optional(parser* p, lxtype type)
{
    if (!is_empty(p) && peek(p)->type == type) {
        eat(p);
        return true;
    } else {
        return false;
    }
}

boolean is_empty(parser* p)
{
    return p->position >= p->tokens.size || peek(p)->type == LX_EOF;
}

// ---------- PARSER METHODS ----------

astnode* astnode_new(const char* value, asttype type, lxpos pos) {
    astnode* node = (astnode*)malloc(sizeof(astnode));
    node->value = value;
    node->type = type;
    node->children = vector_new(4);
    node->pos = pos;
    return node;
}

astnode* parse(parser* p)
{
    return parse_expression(p);
}

astnode* parse_expression(parser* p)
{
    vector primaries = vector_new(8);
    vector operators = vector_new(8);

    vector_push(&primaries, parse_primary(p));

    // Non-primary expression
    while (!is_empty(p) && peek(p)->type == LX_OPERATOR) 
    {
        lxtoken* op = eat(p);

        // Applies shunting yard algorithm
        while (operators.size > 0 && precedence(p, op) <= precedence(p, vector_top(&operators))) 
        {
            vector_push(&primaries, apply_op(&primaries, &operators));
        }

        vector_push(&operators, op);
        vector_push(&primaries, parse_primary(p));
    }

    // Applies remaining operations
    while (operators.size > 0) 
    {
        vector_push(&primaries, apply_op(&primaries, &operators));
    }

    return vector_pop(&primaries);
}

astnode* parse_primary(parser* p)
{
    if (is_empty(p)) {
        parsererror(p, "Program has ended prematurely!");
    }

    astnode* node = (astnode*)malloc(sizeof(astnode));
    node->pos = clone_pos(&peek(p)->pos);
    node->children = vector_new(1);

    switch (peek(p)->type)
    {
        case LX_INTEGER:
            node->type = AST_INTEGER;
            node->value = eat(p)->value;
            break;
        
        case LX_SYMBOL:
            node->type = AST_REFERENCE;
            node->value = eat(p)->value;
            break;
        
        case LX_CALL:
            free(node);
            node = parse_function_call(p);
            break;

        case LX_LEFT_PAREN:
            free(node);
            consume(p, LX_LEFT_PAREN);
            node = parse_expression(p);
            consume(p, LX_RIGHT_PAREN);
            break;

        case LX_OPERATOR:
            // validates operator
            if (strcmp(peek(p)->value, "-") && strcmp(peek(p)->value, "+")) 
            {
                parsererror(p, "Invalid unary operator");
            }

            node->type = AST_UNARY_EXPRESSION;
            node->value = eat(p)->value;
            vector_push(&node->children, parse_primary(p));
            break;

        default:
            parsererror(p, "Failed to parse token");
    }

    return node;
}

astnode* parse_function_call(parser* p)
{
    return NULL;
}

/**
 * Returns an integer value signifying the operator order precedence for
 * the provided operator token. 
 */
int precedence(parser* p, lxtoken* op)
{
    switch (op->value[0])
    {
        case '|':
            return 1;
        case '&':
            return 2;
        case '+':
        case '-':
            return 3;
        case '*':    
        case '/':    
        case '%':
            return 4;    
        default:
            parsererror(p, "Unknown operator recieved");
    }
    return 0;
}

/**
 * Applies binary operation by forming a binary Abstract syntax tree
 * with the operator as the root and the operands as the children. 
 */
astnode* apply_op(vector* operands, vector* operators)
{
    lxtoken *op, *v0, *v1;
    op = (lxtoken*) vector_pop(operators);
    v1 = (lxtoken*) vector_pop(operands);
    v0 = (lxtoken*) vector_pop(operands);

    astnode* expression = astnode_new(op->value, AST_BNARY_EXPRESSION, clone_pos(&op->pos));
    vector_push(&expression->children, v0);
    vector_push(&expression->children, v1);
    return expression;
}

const char* astnode_tostr(astnode* node)
{
    if (node->children.size == 0) {
        return node->value;
    }

    char buf[10000];
    sprintf(buf, node->type == AST_BLOCK ? "[" : "(%s", node->value);
    
    for (size_t i = 0; i < node->children.size; i++) 
    {
        astnode* child = vector_get(&node->children, i);
        sprintf(buf + strlen(buf), " %li:%s", i, astnode_tostr(child));
    }

    sprintf(buf + strlen(buf), node->type == AST_BLOCK ? "]" : ")");

    char* out = (char*)malloc(sizeof(char) * strlen(buf));
    strcpy(out, buf);
    return out;
}

void parsererror(parser* p, const char* msg) 
{
    lxtoken* tk = peek(p);
    fprintf(stderr, "%s[err] %s (%d, %d):\n", ERR_COL, msg, tk->pos.line_pos + 1, tk->pos.col_pos + 1);
    fprintf(stderr, "\t|\n");
    fprintf(stderr, "\t| %04i %s\n", tk->pos.line_pos, get_line(p->source, tk->pos.line_offset));
    fprintf(stderr, "\t| %s^\n%s", paddchar('~', 5 + tk->pos.col_pos), DEF_COL);
    exit(0);
}