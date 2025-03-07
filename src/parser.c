#include "parser.h"

int precedence(parser* p, lxtoken* op);
astnode* apply_op(vector* primaries, vector* operators);
void strip_newlines(parser* p);
astnode* astnode_new(const char* value, asttype type, lxpos pos);


// ------------------ TOKEN TRAVERSAL ------------------

lxtoken* peek(parser* p)
{
    return vector_get(&p->tokens, p->position);
}

lxtoken* lookahead(parser* p)
{
    if (p->position + 1 < p->tokens.size)
        return vector_get(&p->tokens, p->position + 1);
    else
        return NULL;
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

// ------------------ PARSING METHODS ------------------

astnode* parse(parser* p)
{
    astnode* tree = parse_block(p, LX_EOF);
    return tree;
}

astnode* parse_block(parser* p, lxtype terminal)
{
    astnode* block = astnode_new("block", LX_BLOCK, clone_pos(&peek(p)->pos));

    strip_newlines(p);
    
    while (!is_empty(p) && peek(p)->type != terminal) 
    {
        astnode* st = parse_statement(p);
        vector_push(&block->children, st);
        strip_newlines(p);
    }

    return block;
}

astnode* parse_statement(parser* p)
{
    astnode* st = astnode_new(NULL, AST_ASSIGN, clone_pos(&peek(p)->pos));

    switch (peek(p)->type)
    {
        case LX_SYMBOL:
            if (lookahead(p)->type == LX_LEFT_SQUARE || lookahead(p)->type == LX_DOT) {
                free(st);
                st = parse_table_put(p);
            } else {
                st->type = AST_ASSIGN;
                st->value = eat(p)->value;
                consume(p, LX_ASSIGN);
                vector_push(&st->children, parse_expression(p));
            }
            break;

        case LX_CALL:
            free(st);
            st = parse_function_call(p);
            break;
        
        case LX_LOOP:
            free(st);
            st = parse_loop(p);
            break;
        
        case LX_IF:
            free(st);
            st = parse_branching(p);
            break;
        
        case LX_INCLUDE:
            eat(p);
            st->type = AST_INCLUDE;
            st->value = "include";
            
            astnode* fp = parse_primary(p);
            if (fp->type != AST_STRING) {
                parsererror(p, "Expected string in include statement!");
            }

            vector_push(&st->children, fp);
            break;

        case LX_RETURN:
            eat(p);
            st->type = AST_RETURN;
            st->value = "ret";
            vector_push(&st->children, parse_expression(p));
            break;

        default:
            parsererror(p, "Invalid statement!");
            break;
    }

    return st;
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

        case LX_FLOAT:
            node->type = AST_FLOAT;
            node->value = eat(p)->value;
            break;
        
        case LX_BOOL:
            node->type = AST_BOOL;
            node->value = eat(p)->value;
            break;
        
        case LX_STRING:
            node->type = AST_STRING;
            node->value = eat(p)->value;
            break;

        case LX_NULL:
            node->type = AST_NULL;
            node->value = eat(p)->value;
            break;

        case LX_LEFT_BRACE:
            free(node);
            node = parse_table_instance(p);
            break;
        
        case LX_SYMBOL:
            if (lookahead(p)->type == LX_LEFT_SQUARE || lookahead(p)->type == LX_DOT) {
                free(node);
                node = parse_table_get(p);
            } else {
                node->type = AST_REFERENCE;
                node->value = eat(p)->value;
            }
            break;
            
        case LX_FUNCTION:
            free(node);
            node = parse_function_definition(p);
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
            // validates operator as unary
            if (strcmp(peek(p)->value, "-") && strcmp(peek(p)->value, "+") 
                    && strcmp(peek(p)->value, "!") && strcmp(peek(p)->value, "~")) {
                parsererror(p, "Invalid unary operator");
            }

            node->type = AST_UNARY_EXPRESSION;
            node->value = eat(p)->value;
            vector_push(&node->children, parse_primary(p));
            break;

        default:
            parsererror(p, "Unexpected token found");
    }

    return node;
}

astnode* parse_function_call(parser* p)
{
    consume(p, LX_CALL);

    astnode* fcall = (astnode*) malloc(sizeof(astnode));
    fcall->type = AST_CALL;
    fcall->pos = clone_pos(&peek(p)->pos);
    fcall->value = peek(p)->value;
    fcall->children = vector_new(4);
    vector_push(&fcall->children, parse_expression(p));

    consume(p, LX_LEFT_PAREN);

    if (!is_empty(p) && peek(p)->type != LX_RIGHT_PAREN) 
    {
        do 
        {
            vector_push(&fcall->children, parse_expression(p));
        } 
        while (consume_optional(p, LX_SEPARATOR));
    }

    consume(p, LX_RIGHT_PAREN);

    return fcall;
}

astnode* parse_function_definition(parser* p)
{
    astnode* func = astnode_new("code", AST_FUNCTION, clone_pos(&consume(p, LX_FUNCTION)->pos));;
    astnode* params = astnode_new("args", AST_PARAMS, clone_pos(&consume(p, LX_LEFT_PAREN)->pos));
    vector_push(&func->children, params);

    // code header
    if (peek(p)->type != LX_RIGHT_PAREN) 
    {
        do {
            lxtoken* param = consume(p, LX_SYMBOL);
            vector_push(&params->children, astnode_new(param->value, AST_PARAM, clone_pos(&param->pos)));
        } 
        while (consume_optional(p, LX_SEPARATOR));
    }
    consume(p, LX_RIGHT_PAREN);

    // code body
    strip_newlines(p);
    consume(p, LX_LEFT_BRACE);
    vector_push(&func->children, parse_block(p, LX_RIGHT_BRACE));
    consume(p, LX_RIGHT_BRACE);

    return func;
}

astnode* parse_loop(parser* p)
{
    astnode* loop = astnode_new("loop", AST_LOOP, clone_pos(&consume(p, LX_LOOP)->pos));
    
    // loop condition
    vector_push(&loop->children, parse_expression(p));

    // loop body
    strip_newlines(p);
    consume(p, LX_LEFT_BRACE);
    vector_push(&loop->children, parse_block(p, LX_RIGHT_BRACE));
    consume(p, LX_RIGHT_BRACE);

    return loop;
}

astnode* parse_branching(parser* p)
{
    astnode* branch0 = astnode_new("conditional", AST_BRANCHES, clone_pos(&consume(p, LX_IF)->pos));

    // if condition { ... } branch
    vector_push(&branch0->children, parse_expression(p));
    
    strip_newlines(p);
    consume(p, LX_LEFT_BRACE);
    vector_push(&branch0->children, parse_block(p, LX_RIGHT_BRACE));
    consume(p, LX_RIGHT_BRACE);
    strip_newlines(p);

    // else and else if { ... } branches
    astnode* branchx = branch0;
    while (peek(p)->type == LX_ELSE)
    {
        astnode* branch = astnode_new(NULL, AST_BRANCHES, clone_pos(&eat(p)->pos));

        if (consume_optional(p, LX_IF)) {
            branch->value = "conditional";
            vector_push(&branch->children, parse_expression(p));
        } else {
            branch->value = "alt";
        }
        
        strip_newlines(p);
        consume(p, LX_LEFT_BRACE);
        vector_push(&branch->children, parse_block(p, LX_RIGHT_BRACE));
        consume(p, LX_RIGHT_BRACE);
        strip_newlines(p);
        
        // recursive if statement
        vector_push(&branchx->children, branch);
        branchx = branch;

        if (streq(branch->value, "alt")) break;
    }

    return branch0;   
}

astnode* parse_table_instance(parser* p)
{
    astnode* table = astnode_new("table", AST_TABLE, clone_pos(&consume(p, LX_LEFT_BRACE)->pos));
    strip_newlines(p);

    // parses table entries
    if (peek(p)->type != LX_RIGHT_BRACE) 
    {
        do {
            strip_newlines(p);
            astnode* element = astnode_new("pair", AST_KV_PAIR, clone_pos(&peek(p)->pos));
            vector_push(&element->children, parse_expression(p));
            consume(p, LX_COLON);
            vector_push(&element->children, parse_expression(p));
            strip_newlines(p);
            vector_push(&table->children, element);
        } 
        while (consume_optional(p, LX_SEPARATOR));
    }
    
    consume(p, LX_RIGHT_BRACE);
    return table;
}

astnode* parse_table_put(parser* p)
{
    lxtoken* var = consume(p, LX_SYMBOL);
    astnode* put = astnode_new(var->value, AST_PUT, clone_pos(&var->pos));


    if (consume_optional(p, LX_LEFT_SQUARE)) {
        vector_push(&put->children, parse_expression(p));
        consume(p, LX_RIGHT_SQUARE);
    }
    else if (consume_optional(p, LX_DOT))
    {
        lxtoken* tk = consume(p, LX_SYMBOL);
        vector_push(&put->children, astnode_new(tk->value, AST_STRING, clone_pos(&tk->pos)));
    }

    consume(p, LX_ASSIGN);
    vector_push(&put->children, parse_expression(p));
    return put;
}

astnode* parse_table_get(parser* p)
{
    lxtoken* var = consume(p, LX_SYMBOL);
    astnode* get = astnode_new(var->value, AST_GET, clone_pos(&var->pos));

    if (consume_optional(p, LX_LEFT_SQUARE))
    {
        vector_push(&get->children, parse_expression(p));
        consume(p, LX_RIGHT_SQUARE);
    }
    else if (consume_optional(p, LX_DOT))
    {
        lxtoken* tk = consume(p, LX_SYMBOL);
        vector_push(&get->children, astnode_new(tk->value, AST_STRING, clone_pos(&tk->pos)));
    }

    return get;
}

// ------------------ UTILITY METHODS ------------------

astnode* astnode_new(const char* value, asttype type, lxpos pos) 
{
    astnode* node = (astnode*)malloc(sizeof(astnode));
    node->value = value;
    node->type = type;
    node->children = vector_new(4);
    node->pos = pos;
    return node;
}

/**
 * Returns an integer value signifying the operator order precedence for
 * the provided operator token. 
 */
int precedence(parser* p, lxtoken* op)
{
    if (streq(op->value, "<=") || streq(op->value, ">="))
        return 8;
    else if (streq(op->value, "==") || streq(op->value, "!="))
        return 7;
    else if (streq(op->value, "&&"))
        return 3;
    else if (streq(op->value, "||"))
        return 2;
    
    switch (op->value[0])
    {
        case '|':
            return 4;
        case '^':
            return 5;
        case '&':
            return 6;
        case '<':
        case '>':
            return 8;
        case '+':
        case '-':
            return 9;
        case '*':    
        case '/':    
        case '%':
            return 10;    
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

    astnode* expression = astnode_new(op->value, AST_BINARY_EXPRESSION, clone_pos(&op->pos));
    vector_push(&expression->children, v0);
    vector_push(&expression->children, v1);
    return expression;
}

void strip_newlines(parser* p)
{
    while (consume_optional(p, LX_NEWLINE));
}

#ifdef HE_DEBUG_MODE
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
#endif

void parsererror(parser* p, const char* msg) 
{
    lxtoken* tk = peek(p);
    fprintf(stderr, "%s[err] %s (%d, %d) in %s:\n", ERR_COL, msg, tk->pos.line_pos + 1, tk->pos.col_pos + 1, tk->pos.origin);
    fprintf(stderr, "\t|\n");
    fprintf(stderr, "\t| %04i %s\n", tk->pos.line_pos + 1, get_line(p->source, tk->pos.line_offset));
    fprintf(stderr, "\t| %s'\n%s", paddchar('~', 5 + tk->pos.col_pos), DEF_COL);
    exit(0);
}