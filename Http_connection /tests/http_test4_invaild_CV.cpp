#include<iostream>
#include "../HTTPConnectionBuilder.h" 
int main() {
    HTTPBuilder connection_builder;

    (connection_builder->*&HTTPConnectionBuilder::add_header)("Content-Type: application/json");
    (connection_builder->*&HTTPConnectionBuilder::add_body)("{ \"name\": \"Invalid4\", \"age\": 30 }");
    auto connection4a = (connection_builder->*&HTTPConnectionBuilder::build)();
    // Invalid: Trying to build again without starting a new process
    auto connection4b = (connection_builder->*&HTTPConnectionBuilder::build)();

    return 0;
}
