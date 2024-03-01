#include<iostream>
#include "./HTTPConnectionBuilder.h" 
int main() {
    HTTPBuilder connection_builder;

    // Adding headers
    (connection_builder->*&HTTPConnectionBuilder::add_header)("Content-Type: application/json");
    (connection_builder->*&HTTPConnectionBuilder::add_header)("Accept: application/json");
    (connection_builder->*&HTTPConnectionBuilder::add_header)("Authorization: Bearer YOUR_TOKEN_HERE");

    // Adding body
    (connection_builder->*&HTTPConnectionBuilder::add_body)("{ \"name\": \"John Doe\", \"age\": 30 }");

    // Building the connection
    auto connection = (connection_builder->*&HTTPConnectionBuilder::build)();

    // Utilize the built connection here...
    return 0;
}
