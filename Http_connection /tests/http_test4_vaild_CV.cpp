#include<iostream>
#include "../HTTPConnectionBuilder.h" 
int main() {
    HTTPBuilder connection_builder;

    (connection_builder->*&HTTPConnectionBuilder::add_header)("Content-Type: application/json");
    (connection_builder->*&HTTPConnectionBuilder::add_body)("{ \"name\": \"Bob\", \"age\": 22 }");
    auto connection4a = (connection_builder->*&HTTPConnectionBuilder::build)();

    // Starting a new connection building process
    (connection_builder->*&HTTPConnectionBuilder::add_header)("Content-Type: text/plain");
    (connection_builder->*&HTTPConnectionBuilder::add_body)("This is plain text.");
    auto connection4b = (connection_builder->*&HTTPConnectionBuilder::build)();

    return 0;
}
