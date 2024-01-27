#include<iostream>
#include "HTTPConnectionBuilder.h" 
int main() {
    HTTPBuilder connection_builder;

    // Calling the member functions on the object using the pointer-to-member-function syntax
    (connection_builder->*&HTTPConnectionBuilder::add_header)("Content-Type: application/json");
    (connection_builder->*&HTTPConnectionBuilder::add_header)("Authorization: Bearer YOUR_TOKEN_HERE");
    (connection_builder->*&HTTPConnectionBuilder::add_body)("{ \"name\": \"John Doe\", \"age\": 30 }");
    (connection_builder->*&HTTPConnectionBuilder::build)();

    return 0;
}
