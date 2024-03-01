#include<iostream>
#include "../HTTPConnectionBuilder.h" 

int main() {
    HTTPBuilder connection_builder;

    (connection_builder->*&HTTPConnectionBuilder::add_body)("{ \"name\": \"Invalid1\", \"age\": 30 }");
    (connection_builder->*&HTTPConnectionBuilder::add_header)("Content-Type: application/json");
    auto connection1 = (connection_builder->*&HTTPConnectionBuilder::build)();

    return 0;
}
