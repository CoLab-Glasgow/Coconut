#include<iostream>
#include "../HTTPConnectionBuilder.h" 
int main() {
    HTTPBuilder connection_builder;

    (connection_builder->*&HTTPConnectionBuilder::add_body)("{ \"name\": \"Invalid3\", \"age\": 30 }");
    (connection_builder->*&HTTPConnectionBuilder::add_header)("Content-Type: application/json");
    (connection_builder->*&HTTPConnectionBuilder::add_header)("Accept: application/json");
    auto connection3 = (connection_builder->*&HTTPConnectionBuilder::build)();

    return 0;
}
