After tokenizing command line arguments, next, the values of the
argv must be pushed into the stack. 

Normally in the stack we push argument values from
right to left. 

for instance if the function is 
int func (int a, int b, int c); 
Then it is necessary to follow orders in the following way 
c, ,b, a

After pushing all the argv values, we need to push the argc.
Thus we are laying out the ground for calling the function considering the
command line arguments. 

Plan how to proceed;
as already argv[] has the tokens, now using memcpy we copy
each argv[] value to the esp but before we need to do 
esp - (size of the argv[i])



This changes should be done in setup_stack

/*
static bool setup_stack (void **esp) **
*/