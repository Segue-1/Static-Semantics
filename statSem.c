#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#include "stack.h"
#include "scanner.h"
#include "token.h"
#include "parser.h"

void statSem(char * input) {

	struct Token token;
	int left = 0, right = 0, line_number = 0;
	int * left_ptr = &left;
	int * right_ptr = &right;
	int * line_numberPtr = &line_number;
	struct node_t* root;
	struct token_Stack * globals_stack; 
	struct token_Stack * locals_stack; 
	struct token_Stack * used_stack; 
	struct token_Stack * used_locals_stack; 



	//const char *token_Types[] = { "operator/delim_tK" , "identifier_tK" , "number_tK" , "EOF_tK", "keyword_tK" };


	// Get first token
	token = scanner(input, left_ptr, right_ptr, line_numberPtr);

	// Stack of global declarations
	globals_stack = createStack(100);

	// Stack of local declarations
	locals_stack = createStack(100);

	// Used variables within entire program
	used_stack = createStack(100);

	// Used variables within a block
	used_locals_stack = createStack(100);

	// Calls program, will recursively parse tokens
	root = program(&token, input, left_ptr, right_ptr, line_numberPtr, globals_stack, locals_stack, used_stack, used_locals_stack);

	// Calculators levels of nodes then prints them with preorder traversal
	calc_depth(root, 0);
	printPreorder(root);

	if (token.type == 3) {
		printf("Final token is %s\n", token.instance);
		printf("Parse successful\n");
	}
	else {
		printf("Final token is %s\n Expecting EOF token\n", token.instance);
		printf("Parse failed\n");
	}
	

	exit(0);
}



struct node_t* program(struct Token* token, char * str, int * left, int * right, int * line_number, struct token_Stack* globals_stack, struct token_Stack* locals_stack, struct token_Stack* used_stack, struct token_Stack* used_locals_stack){

	char label[] = "program";
	struct node_t* node;

	// Check for main keyword
	if (strcmp(token->instance, "main") == 0){
		node = get_Node(token, label);
	}
	else {
		node = gen_Node(label);
	}


	// Call vars on left child and check globals in vars (before main)
	node->left_child = vars(token, str, left, right, line_number);
	check_vars(node->left_child, globals_stack);

	
	int i;

	// Check for duplicate global declarations
	int j;
	if (!isEmpty(globals_stack)) {
		for (i = 0; i <= globals_stack->top - 1; i++) {
			for (j = i + 1; j <= globals_stack->top; j++) {
				if (strcmp(globals_stack->array[i].instance, globals_stack->array[j].instance) == 0) {
					printf("Error: Duplicate in global scope\n");
					printf("Identifier %s ", globals_stack->array[i].instance);
					printf("first declared on line %d, redeclared on line %d\n", globals_stack->array[i].line_number + 1, globals_stack->array[j].line_number + 1);
					exit(0);
				}
			}
		}
	}



	// If 'main' token, then consume
	if (strcmp(token->instance, "main") == 0){
		(*token) = scanner(str, left, right, line_number);
	}


	// Call block on middle child
	node->middle_child = block(token, str, left, right, line_number);


	// Used for maintaining stack
	int * varCount = malloc(sizeof(int));
	int * local_varCount = malloc(sizeof(int));
	(*varCount) = 0;
	(*local_varCount) = 0;


	// Build a stack of variables used after first block
	gen_usedStack(node->middle_child, used_stack);


	// Check globals in block (after main)
	check_local_vars(node->middle_child, locals_stack, varCount, globals_stack, used_stack, used_locals_stack, local_varCount);





	// Final check for globals in local scopes
	int d;
	if (!isEmpty(locals_stack) && !isEmpty(globals_stack)) {
		for (i = 0; i <= globals_stack->top; i++) {		
			d = find(locals_stack, globals_stack->array[i].instance);
			if (d != -1) {
				printf("Error: identifier %s declared on line %d\n", globals_stack->array[i].instance, globals_stack->array[i].line_number + 1);
				printf("Redeclared on line %d\n", locals_stack->array[(locals_stack->top - d) + 1].line_number + 1);
				exit(0);
			}  
		}
	}

	if (!isEmpty(used_stack)) {
		for (i = 0; i <= globals_stack->top; i++) {		
			d = find(used_stack, globals_stack->array[i].instance);
			if (d == -1) {
				printf("Error: identifier %s declared on line %d\n", globals_stack->array[i].instance, globals_stack->array[i].line_number + 1);
				printf("Declared but never used\n");
				exit(0);
			}  
		}
	}

	//process_used_local_final(locals_stack, varCount, used_stack, globals_stack, local_varCount);


	return node;

}



struct node_t* block(struct Token* token, char * str, int * left, int * right, int * line_number){

	char label[] = "block";
	struct node_t* node;


	// If 'begin' token then consume
	if (strcmp(token->instance, "begin") == 0){
		node = get_Node(token, label);
		(*token) = scanner(str, left, right, line_number);
	}
	else {
		printf("Error (block): Expecting begin keyword after main\n");
		exit(0);
	}


	// Call vars on left child
	node->left_child = vars(token, str, left, right, line_number);


	// Call stats on middle child
	node->middle_child = stats(token, str, left, right, line_number);



	// If 'end' token then consume
	if (strcmp(token->instance, "end") == 0){
		return node;
	}	


	return node;

}



struct node_t* vars(struct Token* token, char * str, int * left, int * right, int * line_number){

	char label[] = "vars";
	struct node_t* node;	


	// If token instance is not data, return
	if (strcmp(token->instance, "data") != 0){
		return NULL;
	}


	// Check for data token and store if it is there
	if(strcmp(token->instance, "data") == 0){
		node = get_Node(token, label);
		(*token) = scanner(str, left, right, line_number);
	}
	else{
		printf("Error (vars): Expecting data token\n");
		exit(0);
	}


	// If identifier store in node
	if (token->type == 1) {

		store_second_token(node, token);
		(*token) = scanner(str, left, right, line_number);

		// Check for := after data
		if(strcmp(token->instance, ":=") != 0) {
			printf("token: %s\n", token->instance);
			printf("Error (vars): expecting := after data\n");
			exit(0);
		}
		// Consume = token
		(*token) = scanner(str, left, right, line_number);	
	}
	else {
		printf("Error (vars): Expecting identifier after :=\n");
		exit(0);
	}


	// If number store second token in same node
	if(token->type == 2) {
		store_third_token(node, token);
		(*token) = scanner(str, left, right, line_number);
	}
	else{
		printf("Error (vars): expecting number token after :=\n");
		exit(0);
	}


	// Check for ; at the end
	if(strcmp(token->instance, ";") == 0){
		(*token) = scanner(str, left, right, line_number);

	}
	else  {
		printf("Error (vars): expecting ; token after number\n");
		exit(0);
	}


	// Call vars with left child
	node->left_child = vars(token, str, left, right, line_number);

	
	return node;

}


struct node_t* expr(struct Token* token, char * str, int * left, int * right, int * line_number){

	char label[] = "expr";

	// Generate node, get lookahead
	struct node_t* node = gen_Node(label);
	struct Token lookahead = next_Token(str, left, right, line_number);


	// If lookahead is "-" then  call expr
	if (strcmp(lookahead.instance, "-") == 0){
		(*token) = scanner(str, left, right, line_number);
		(*token) = scanner(str, left, right, line_number);
		node->left_child = expr(token, str, left, right, line_number);
	}
	else{
		// If lookahead is something else, call N
		node->left_child = N(token, str, left, right, line_number);

	}


	return node;

}


struct node_t* N(struct Token* token, char * str, int * left, int * right, int * line_number){

	// Generate node, get lookahead
	char label[] = "N";
	struct node_t* node = gen_Node(label);
	struct Token lookahead = next_Token(str, left, right, line_number);

	
	// If next token is "/" call N() with left child.
	// If "*" call N() with left child
	// Else call A() with left child
	if (strcmp(lookahead.instance, "/") == 0){
		(*token) = scanner(str, left, right, line_number);
		(*token) = scanner(str, left, right, line_number);
		node = N(token, str, left, right, line_number);
	}
	else if (strcmp(lookahead.instance, "*") == 0){
		(*token) = scanner(str, left, right, line_number);
		(*token) = scanner(str, left, right, line_number);
		node->left_child = N(token, str, left, right, line_number);
	}
	else {
		node->left_child = A(token, str, left, right, line_number);
	}


	return node;

}


struct node_t* A(struct Token* token, char * str, int * left, int * right, int * line_number){

	// Generate node, get lookahead
	char label[] = "A";
	struct node_t* node = gen_Node(label);
	struct Token lookahead = next_Token(str, left, right, line_number);	
	

	// If next token "+" then call A(), else call M()
	if (strcmp(lookahead.instance, "+") == 0){
		(*token) = scanner(str, left, right, line_number);
		(*token) = scanner(str, left, right, line_number);
		node = A(token, str, left, right, line_number);
	}
	else if (strcmp(lookahead.instance, "-") == 0){
		(*token) = scanner(str, left, right, line_number);
		(*token) = scanner(str, left, right, line_number);
		node->left_child = A(token, str, left, right, line_number);
	}
	else {
		node->left_child = M(token, str, left, right, line_number);

	}

	
	return node;

}


struct node_t* M(struct Token* token, char * str, int * left, int * right, int * line_number){

	// Generate node
	char label[] = "M";
	struct node_t* node = gen_Node(label);
	
	
	// If current token * call M() with left_child, else call R() with left child
	if (strcmp(token->instance, "*") == 0){
		(*token) = scanner(str, left, right, line_number);
		node->left_child = M(token, str, left, right, line_number);
	}
	else {
		node->left_child = R(token, str, left, right, line_number);
	}


	return node;

}


struct node_t* R(struct Token* token, char * str, int * left, int * right, int * line_number){

	// Generate node
	char label[] = "R";
	struct node_t* node = gen_Node(label);


	// If open parenthesis, consume token and call expr, check for ) and return.
	if (strcmp(token->instance, "(") == 0){
		(*token) = scanner(str, left, right, line_number);
		node->left_child = expr(token, str, left, right, line_number);	
	}


	// If identifier, then store token in node
	if (token->type == 1){
		store_token(node, token);
		(*token) = scanner(str, left, right, line_number);
	}
	// If number, then store token in node
	else if (token->type == 2){
		store_token(node, token);
		(*token) = scanner(str, left, right, line_number);

	}


	if (strcmp(token->instance, ")") == 0){
		(*token) = scanner(str, left, right, line_number);
	}


	return node;

}


struct node_t* stats(struct Token* token, char * str, int * left, int * right, int * line_number){

	// Generate node
	char label[] = "stats";
	struct node_t* node = gen_Node(label);

	// Call stats with left child
	node->left_child = stat(token, str, left, right, line_number);
	
	// Call mStat with middle child
	node->middle_child = mStat(token, str, left, right, line_number);


	return node;

}


struct node_t* mStat(struct Token* token, char * str, int * left, int * right, int * line_number){	

	// Generate node and store mStat label
	char label[] = "mStat";
	struct node_t* node = gen_Node(label);


	// Check if lookahead is end
	if (strcmp(token->instance, "end") == 0){
		store_token(node, token);
		(*token) = scanner(str, left, right, line_number);
		return node;
	}


	// Call stat with left child and mStat with middle
	node->left_child = stat(token, str, left, right, line_number);
	node->middle_child = mStat(token, str, left, right, line_number);

	return node;

}


struct node_t* stat(struct Token* token, char * str, int * left, int * right, int * line_number){

	// Generate node and store stat in the label	
	char label[] = "stat";
	struct node_t* node = gen_Node(label);


	// Checks token for keywords and call a function to process
	if(strcmp(token->instance, "getter") == 0){

		node->left_child = in(token, str, left, right, line_number);

		if (strcmp(token->instance, ";") != 0) {
			printf("Expecting ; at the end of getter statement\n");
			exit(0);
		}

		(*token) = scanner(str, left, right, line_number);
	}
	else if(strcmp(token->instance, "outter") == 0){

		node->left_child = out(token, str, left, right, line_number);

		if (strcmp(token->instance, ";") != 0) {
			printf("Expecting ; at the end of outter statement\n");
			exit(0);
		}

		(*token) = scanner(str, left, right, line_number);
	}
	else if(strcmp(token->instance, "begin") == 0){

		node->left_child = block(token, str, left, right, line_number);
	}
	else if(strcmp(token->instance, "if") == 0){

		node->left_child = if_(token, str, left, right, line_number);
		if (strcmp(token->instance, ";") != 0) {
			printf("Expecting ; at the end of if statement\n");
			exit(0);
		}

		(*token) = scanner(str, left, right, line_number);
	}
	else if(strcmp(token->instance, "loop") == 0){

		node->left_child = loop(token, str, left, right, line_number);

		if (strcmp(token->instance, ";") != 0) {
			printf("Expecting ; at the end of loop statement\n");
			exit(0);
		}

		(*token) = scanner(str, left, right, line_number);
	}
	else if(strcmp(token->instance, "assign") == 0){

		node->left_child = assign(token, str, left, right, line_number);

		if (strcmp(token->instance, ";") != 0) {
			printf("Expecting ; at the end of assign statement\n");
			exit(0);
		}

		(*token) = scanner(str, left, right, line_number);
	}
	else if(strcmp(token->instance, "void") == 0){

		node->left_child = label_(token, str, left, right, line_number);

		if (strcmp(token->instance, ";") != 0) {
			printf("Expecting ; at the end of void statement\n");
			exit(0);
		}

		(*token) = scanner(str, left, right, line_number);		
	}
	else if(strcmp(token->instance, "proc") == 0){
		node->left_child = goto_(token, str, left, right, line_number);
		if (strcmp(token->instance, ";") != 0) {
			printf("Expecting ; at the end of proc statement\n");
			exit(0);
		}
		(*token) = scanner(str, left, right, line_number);
	}
	// Consume purely structure tokens if they get down here
	else if(strcmp(token->instance, "main") == 0){
		(*token) = scanner(str, left, right, line_number);
	}
	else if(strcmp(token->instance, "begin") == 0){
		(*token) = scanner(str, left, right, line_number);
	}
	else {
		printf("STAT token.instance: %s\n", token->instance);
		printf("Error (stat): Expecting keyword\n");
		exit(0);
	}


	return node;

}


struct node_t* in(struct Token* token, char * str, int * left, int * right, int * line_number){

	// Generate node and store in token with label
	char label[] = "in";
	struct node_t* node = get_Node(token, label);

	// Consume in token
	(*token) = scanner(str, left, right, line_number);	

	// Check for identifier
	if (token->type == 1) {

		// Store then consume identifier token		
		store_second_token(node, token);
		(*token) = scanner(str, left, right, line_number);

	}
	else if (token->type != 1) {
		printf("Error (in): Expecting Identifier after getter");
		exit(0);
	}

	return node;

}


struct node_t* out(struct Token* token, char * str, int * left, int * right, int * line_number){

	// Generate node, store out label and outter token
	char label[] = "out";
	struct node_t* node = get_Node(token, label);

	// Consume outter token
	(*token) = scanner(str, left, right, line_number);


	// Call expr on left child
	node->left_child = expr(token, str, left, right, line_number);

	return node;
}


struct node_t* if_(struct Token* token, char * str, int * left, int * right, int * line_number){

	// Generate node, store if  label, and store if token
	char label[] = "if";
	struct node_t* node = get_Node(token, label);

	// Consume if
	(*token) = scanner(str, left, right, line_number);

	// If next token is [ call expr, RO, and expr on children and consume token
	if (strcmp(token->instance, "[") == 0){
		node->left_child = expr(token, str, left, right, line_number);
		node->middle_child = RO(token, str, left, right, line_number);
		node->right_child = expr(token, str, left, right, line_number);
	}
	else {
		printf("Error (if_): Expecting open bracket after if keyword.\n");
		exit(0);
	}


	// If ] consume and get new lookahead.
	if (strcmp(token->instance, "]") == 0){
		(*token) = scanner(str, left, right, line_number);
	}
	else {
		printf("Error (if_): Expecting closed bracket.\n");
		exit(0);
	}


	// If next token is "then", consume and call stat with far right child
	if (strcmp(token->instance, "then") == 0){
		store_second_token(node, token);
		(*token) = scanner(str, left, right, line_number);
		node->far_right_child = stat(token, str, left, right, line_number);
			
	}


	return node;

}


struct node_t* loop(struct Token* token, char * str, int * left, int * right, int * line_number){

	// Generate node, store if  label, and store if token
	char label[] = "loop";
	struct node_t* node = get_Node(token, label);

	// Consume loop token and get lookahead
	(*token) = scanner(str, left, right, line_number);

	// Check for [ and consume
	if (strcmp(token->instance, "[") == 0) {

		// Store loop token and call expr, RO, and expr with children
		(*token) = scanner(str, left, right, line_number);
		
		node->left_child = expr(token, str, left, right, line_number);
		node->middle_child = RO(token, str, left, right, line_number);
		node->right_child = expr(token, str, left, right, line_number);
	}
	else {
		printf("Error (loop): Expecting open bracket token.\n");
		exit(0);
	}


	// If token is ] then consume
	if (strcmp(token->instance, "]") == 0){
		(*token) = scanner(str, left, right, line_number);			
	}
	else {
		printf("Error (loop): Expecting closed bracket after open bracket.\n");
		exit(0);
	}

	// Call stat with far right child
	node->far_right_child = stat(token, str, left, right, line_number);


	return node;

}


struct node_t* assign(struct Token* token, char * str, int * left, int * right, int * line_number){

	// Generate node, store assign label, store assign token, then consume
	char label[] = "assign";
	struct node_t* node = get_Node(token, label);
	(*token) = scanner(str, left, right, line_number);


	// If identifier, store in node, and consume token
	if (token->type == 1) {
		store_second_token(node, token);
		(*token) = scanner(str, left, right, line_number);	
	}
	else {
		printf("Erros (assign): Expecting identifier token after assign keywork\n");
		exit(0);
	}


	// If number store in node
	if(strcmp(token->instance, ":=") == 0) {
		(*token) = scanner(str, left, right, line_number);	
		node->left_child = expr(token, str, left, right, line_number);
	}
	else{
		printf("Error (assign): expecting '=' after identifier token\n");
		exit(0);
	}


	return node;

}


struct node_t* RO(struct Token* token, char * str, int * left, int * right, int * line_number){	

	// Generate node, store assign label, and store assign token
	char label[] = "RO";
	struct node_t* node = get_Node(token, label);


	// Parse relational operators
	if(strcmp(token->instance, "=>") == 0){
		(*token) = scanner(str, left, right, line_number);
		return node;
	}
	else if(strcmp(token->instance, "=<") == 0){
		(*token) = scanner(str, left, right, line_number);
		return node;
	}
	else if(strcmp(token->instance, "==") == 0){
		(*token) = scanner(str, left, right, line_number);
		return node;
	}
	else if(strcmp(token->instance, "[") == 0){
	(*token) = scanner(str, left, right, line_number);
		if(strcmp(token->instance, "==") == 0){
			store_second_token(node, token);
			(*token) = scanner(str, left, right, line_number);
		}
		else {
			printf("Error (RO): expecting '==' after open bracket\n");
			exit(0);
		}

		if(strcmp(token->instance, "]") == 0){
			store_third_token(node, token);
		}
		else {
			printf("Error (RO): expecting closed bracket after ==\n");
			exit(0);
		}
	}
	else if(strcmp(token->instance, "%") == 0){
		(*token) = scanner(str, left, right, line_number);
		return node;
	}
	else if(strcmp(token->instance, "=") == 0){
		(*token) = scanner(str, left, right, line_number);
		return node;
	}


	return node;
}


struct node_t* label_(struct Token* token, char * str, int * left, int * right, int * line_number){

	// Generate node, store label label, store void token, then consume
	char label[] = "label";
	struct node_t* node = get_Node(token, label);
	(*token) = scanner(str, left, right, line_number);


	// If identifier store in node
	if (token->type == 1) {
		store_second_token(node, token);
		(*token) = scanner(str, left, right, line_number);
	}
	else {
		printf("Error (label_): Expecting identifier token after void keyword\n");
		exit(0);
	}


	return node;

}


struct node_t* goto_(struct Token* token, char * str, int * left, int * right, int * line_number){

	// Generate node, store goto label, store proc token, then consume
	char label[] = "goto";
	struct node_t* node = get_Node(token, label);
	(*token) = scanner(str, left, right, line_number);


	// If identifier store in node
	if (token->type == 1) {
		store_second_token(node, token);	
		(*token) = scanner(str, left, right, line_number);
	}
	else {
		printf("Error (goto_): Expecting identifier after proc keyword\n");
		exit(0);
	}


	return node;

}


// This function stores a token and sets children  to null.
// Use store_token() if you want to only store a token.
struct node_t* get_Node(struct Token* token, char * label){
	
	// Allocate space for note_t object
	struct node_t* node = (struct node_t*)malloc(sizeof(struct node_t));

	// Allocate space for token instance string and copy from token to node's token
	node->token.instance = malloc(strlen(token->instance) * sizeof (char));;
	strcpy(node->token.instance, token->instance);

	// Copy token type and line number
	node->token.type = token->type;
	node->token.line_number = token->line_number;

	// Set children to null
	node->left_child = NULL;
	node->middle_child = NULL;
	node->right_child = NULL;
	node->far_right_child = NULL;

	// Set flags, since we stored one token, tkflg1 = 1
	node->tkflg1 = 1;
	node->tkflg2 = 0;
	node->tkflg3 = 0;


	// Allocate space for token instance string and copy from token to node's token
	node->label = malloc(strlen(label) * sizeof (char));
	strcpy(node->label, label);


	return node;

}


// Simply allocates space for node and sets children to null
struct node_t* gen_Node(char * label){
	
	// Allocate space for note_t's token and copy token to it
	struct node_t* node = (struct node_t*)malloc(sizeof(struct node_t));

	// Set children to null
	node->left_child = NULL;
	node->middle_child = NULL;
	node->right_child = NULL;
	node->far_right_child = NULL;

	// Allocate space for label string and copy
	node->label = malloc(strlen(label) * sizeof (char));
	strcpy(node->label, label);

	// Set flags to zero
	node->tkflg1 = 0;
	node->tkflg2 = 0;
	node->tkflg3 = 0;


	return node;

}


// Just stores second token in node
void store_second_token(struct node_t* node, struct Token* token){
	
	// Allocate space for note_t's token and copy token to it
	//node = (struct node_t*)malloc(sizeof(struct node_t));

	// Allocate space for token instance string and copy from token to node's token
	node->second_token.instance = malloc(strlen(token->instance) * sizeof (char));
	strcpy(node->second_token.instance, token->instance);

	// Copy token type and line number
	node->second_token.type = token->type;
	node->second_token.line_number = token->line_number;

	node->tkflg2 = 1;

}


// Just stores third token in node
void store_third_token(struct node_t* node, struct Token* token){
	
	// Allocate space for note_t's token and copy token to it
	//node = (struct node_t*)malloc(sizeof(struct node_t));

	// Allocate space for token instance string and copy from token to node's token
	node->third_token.instance = malloc(strlen(token->instance) * sizeof (char));
	strcpy(node->third_token.instance, token->instance);

	// Copy token type and line number
	node->third_token.type = token->type;
	node->third_token.line_number = token->line_number;

	node->tkflg3 = 1;

}

// Stores token in node
void store_token(struct node_t* node, struct Token* token){
	
	// Allocate space for note_t's token and copy token to it
	//node = (struct node_t*)malloc(sizeof(struct node_t));

	// Allocate space for token instance string and copy from token to node's token
	node->token.instance = malloc(strlen(token->instance) * sizeof (char));
	strcpy(node->token.instance, token->instance);

	// Copy token type and line number
	node->token.type = token->type;
	node->token.line_number = token->line_number;

	node->tkflg1 = 1;

}

// Lookahead function, calls scanner and resets to previous values, as if it never got next token.
struct Token next_Token(char * str, int * left, int * right, int * line_number){

	int temp_left; 
	int temp_right;
	int temp_line_number;

	temp_left = (*left);
	temp_right = (*right);
	temp_line_number = (*line_number);


	struct Token next;
	next = scanner(str, left, right, line_number);


	// Reset parameters for scanner
	(*left) = temp_left;
	(*right) = temp_right;
	(*line_number) = temp_line_number;

	return next;

}


// Recursive function which calculates depth of each node
void calc_depth(struct node_t* node, int depth){
	
	if (node != NULL) {

		// Check if node has at least one token stored
		if ((node->tkflg1 == 1 && node->tkflg2 == 0 && node->tkflg3 == 0) || 
		    (node->tkflg1 && node->tkflg2 == 1 && node->tkflg3 == 0) || 
		    (node->tkflg1 && node->tkflg2 == 1 && node->tkflg3 == 1)){

			node->tree_level = depth;

			calc_depth(node->left_child, depth + 1);
			calc_depth(node->middle_child, depth + 1);
			calc_depth(node->right_child, depth + 1);
			calc_depth(node->far_right_child, depth + 1);
		}
		else {

			calc_depth(node->left_child, depth);
			calc_depth(node->middle_child, depth);
			calc_depth(node->right_child, depth);
			calc_depth(node->far_right_child, depth);
		}
	}
}


// Recursive preorder print function, checks token flags to determine what to print
void printPreorder(struct node_t* node){

	int i = 0;
	
	if (node != NULL) {
		if (node->tkflg1 == 1 && node->tkflg2 == 0 && node->tkflg3 == 0) {
			for (i = 0; i < node->tree_level; i++) {
				printf("  ");
			}
			printf("%s: ", node->label);
			printf("%s\n", node->token.instance);

		}
		if (node->tkflg1 && node->tkflg2 == 1 && node->tkflg3 == 0) {
			for (i = 0; i < node->tree_level; i++) {
				printf("  ");
			}
			printf("%s: ", node->label);
			printf("%s %s\n", node->token.instance, node->second_token.instance);

		}
		if (node->tkflg1 && node->tkflg2 == 1 && node->tkflg3 == 1) {
			for (i = 0; i < node->tree_level; i++) {
				printf("  ");
			}
			printf("%s: ", node->label);
			printf("%s %s %s\n", node->token.instance, node->second_token.instance, node->third_token.instance);

		}

		printPreorder(node->left_child);
		printPreorder(node->middle_child);
		printPreorder(node->right_child);
		printPreorder(node->far_right_child);
	}
}


// Recursive function that builds stack for globals
void check_vars(struct node_t* node, struct token_Stack* token_Stack){
	
	if (node != NULL) {
		if (node->tkflg1 == 1 && node->tkflg2 == 1 && node->tkflg3 == 1) {
			if (strcmp(node->token.instance, "data") == 0) {
				push(token_Stack,node->second_token); 
			}


		}

		check_vars(node->left_child, token_Stack);
		check_vars(node->middle_child, token_Stack);
		check_vars(node->right_child, token_Stack);
		check_vars(node->far_right_child, token_Stack);
	}
}


// Recursive function that builds stack for used identifiers throughout entire program
void gen_usedStack(struct node_t* node, struct token_Stack* token_Stack){
	
	if (node != NULL) {
		if (node->tkflg1 == 1 && node->tkflg2 == 0 && node->tkflg3 == 0) {
			if (node->token.type == 1) {
				push(token_Stack, node->token); 
			}
		}
		if (node->tkflg1 == 1 && node->tkflg2 == 1 && node->tkflg3 == 0) {
			if (node->second_token.type == 1) {
				push(token_Stack, node->second_token); 
			}
		}

		gen_usedStack(node->left_child, token_Stack);
		gen_usedStack(node->middle_child, token_Stack);
		gen_usedStack(node->right_child, token_Stack);
		gen_usedStack(node->far_right_child, token_Stack);
	}
}


// Recursive function that checks for double declarations for locals, also checks for globals redeclared locally
void check_local_vars(struct node_t* node, struct token_Stack* token_Stack, int * varCount, struct token_Stack* globals_stack, struct token_Stack* used_stack, struct token_Stack* used_locals_stack, int * local_varCount){



	
	if (node != NULL) {
		if (node->tkflg1 == 1 && node->tkflg2 == 0 && node->tkflg3 == 0){
			if (strcmp(node->token.instance, "begin") == 0) {
				process_used_local(token_Stack , varCount, used_locals_stack, globals_stack, local_varCount); 
			}
			if (strcmp(node->token.instance, "end") == 0) {
				process_used_local(token_Stack , varCount, used_locals_stack, globals_stack, local_varCount); 
			}
			if (strcmp(node->token.instance, "begin") == 0 && (*varCount) > 0) {
				process_stack(token_Stack, varCount, globals_stack);
			}
			if (node->token.type == 1) {
				(*local_varCount)++;
				push(used_locals_stack, node->token); 
			}
		}
		if (node->tkflg1 == 1 && node->tkflg2 == 1 && node->tkflg3 == 0) {
			if (node->second_token.type == 1) {
				(*local_varCount)++;
				push(used_locals_stack, node->second_token); 				
			}
		}	
		if (node->tkflg1 == 1 && node->tkflg2 == 1 && node->tkflg3 == 1) {
			if (strcmp(node->token.instance, "data") == 0) {
				(*varCount)++;
				push(token_Stack, node->second_token); 		

			}
		}

		check_local_vars(node->left_child, token_Stack, varCount, globals_stack, used_stack, used_locals_stack, local_varCount);
		check_local_vars(node->middle_child, token_Stack, varCount, globals_stack, used_stack, used_locals_stack, local_varCount);
		check_local_vars(node->right_child, token_Stack, varCount, globals_stack, used_stack, used_locals_stack, local_varCount);
		check_local_vars(node->far_right_child, token_Stack, varCount, globals_stack, used_stack, used_locals_stack, local_varCount);
	}

}


// Find duplicates and pop tokens for each local scope
void process_stack(struct token_Stack* token_Stack, int * varCount, struct token_Stack* globals_stack) {

	int i;
	int d = -1;


	// Find duplicates within local scope
	if (!isEmpty(token_Stack) && (*varCount) > 0) {
		int j;
		for (i = 0; i <= token_Stack->top - 1; i++) {
			for (j = i + 1; j <= token_Stack->top; j++) {
				if (strcmp(token_Stack->array[i].instance, token_Stack->array[j].instance) == 0) {
					printf("Error: Duplicate in local scope\n");
					printf("Identifier %s ", token_Stack->array[i].instance);
					printf("declared on line %d, redeclared on line %d\n", token_Stack->array[i].line_number + 1, token_Stack->array[j].line_number + 1);
					exit(0);
				}
			}
		}
	}


	// Find redeclaration of a global variable in some local scope
	if (!isEmpty(globals_stack)) {
		for (i = 0; i <= globals_stack->top; i++) {
			d = find(token_Stack, globals_stack->array[i].instance);
			if (d != -1) {
				printf("Error: identifier %s declared on line %d\n", globals_stack->array[i].instance, globals_stack->array[i].line_number + 1);
				printf("Redeclared on line %d\n", token_Stack->array[(token_Stack->top - d) + 1].line_number + 1);
				exit(0);
			}  
		}
	}


	while ((*varCount) > 0) {
		pop(token_Stack);
		(*varCount)--;
	}

}


// Find duplicates and pop tokens for each local scope
void process_used_local(struct token_Stack* token_Stack, int * varCount, struct token_Stack* used_locals_stack, struct token_Stack* globals_stack, int * local_varCount) {

	int fflg = 1;
	int i;
	int d;

	for (i = 0; i <= used_locals_stack->top; i++){
		printf("used locals: %s\n", used_locals_stack->array[i].instance);
	}
	for (i = 0; i <= token_Stack->top; i++){
		printf("token_Stack: %s\n", token_Stack->array[i].instance);
	}
	for (i = 0; i <= globals_stack->top; i++){
		printf("globals_stack: %s\n", globals_stack->array[i].instance);
	}

	// Find unused variables in local
	if (!isEmpty(used_locals_stack)) {
		for (i = 0; i <= used_locals_stack->top; i++) {
			d = find(token_Stack, used_locals_stack->array[i].instance);
			if (d == -1) {
				d = find(globals_stack, used_locals_stack->array[i].instance);
				if (d == -1) {
					printf("Error: variable %s on line %d used but not declared\n", used_locals_stack->array[i].instance, used_locals_stack->array[i].line_number + 1);
					fflg = 0;
				}
			}
		}

	}

	// Finds declared but not used and outputs warning.
	if (!isEmpty(token_Stack)) {
		for (i = 0; i <= token_Stack->top; i++) {
			d = find(used_locals_stack, token_Stack->array[i].instance);
			if (d == -1) {
				printf("Warning: variable %s on line %d declared but not used\n", token_Stack->array[i].instance, token_Stack->array[i].line_number + 1);
			}
		}
		
	}


	// Finds declared but not used and outputs warning.
	if (!isEmpty(token_Stack)) {
		for (i = 0; i <= token_Stack->top; i++) {
			d = find(globals_stack, token_Stack->array[i].instance);
			if (d != -1) {
				printf("Error: variable %s on line %d declared locally and globally\n", token_Stack->array[i].instance, token_Stack->array[i].line_number + 1);
				fflg = 0;
			}
		}
		
	}


	int j;
	if (!isEmpty(token_Stack)) {
		for (i = 0; i <= token_Stack->top - 1; i++) {
			for (j = i + 1; j <= token_Stack->top; j++) {
				if (strcmp(token_Stack->array[i].instance, token_Stack->array[j].instance) == 0) {
					printf("Error: Duplicate in local scope\n");
					printf("Identifier %s ", token_Stack->array[i].instance);
					printf("first declared on line %d, redeclared on line %d\n", token_Stack->array[i].line_number + 1, token_Stack->array[j].line_number + 1);
					//exit(0);
				}
			}
		}
	}
	

	if (fflg == 0) {
		//exit(0);
	}

	// Pop locally used tokens
	while ((*local_varCount) > 0) {
		pop(used_locals_stack);
		(*local_varCount)--;
	}

	while ((*varCount) > 0) {
		pop(token_Stack);
		(*varCount)--;
	}


	printf("\n");

}


