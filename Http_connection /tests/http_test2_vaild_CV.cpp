#include<iostream>
#include "../HTTPConnectionBuilder.h" 
int main() {
    HTTPBuilder connection_builder;

    (connection_builder->*&HTTPConnectionBuilder::add_header)("Content-Type: application/json");
    (connection_builder->*&HTTPConnectionBuilder::add_body)("{ \"name\": \"Jane\", \"age\": 25 }");
    auto connection2 = (connection_builder->*&HTTPConnectionBuilder::build)();

    return 0;
}
