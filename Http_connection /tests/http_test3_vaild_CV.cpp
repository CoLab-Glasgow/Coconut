#include<iostream>
#include "../HTTPConnectionBuilder.h" 
int main() {
    HTTPBuilder connection_builder;

    (connection_builder->*&HTTPConnectionBuilder::add_header)("Content-Type: application/json");
    (connection_builder->*&HTTPConnectionBuilder::add_header)("Accept-Language: en-US");
    (connection_builder->*&HTTPConnectionBuilder::add_header)("Authorization: Bearer TOKEN_XYZ");
    (connection_builder->*&HTTPConnectionBuilder::add_body)("{ \"name\": \"Alice\", \"age\": 28 }");
    auto connection3 = (connection_builder->*&HTTPConnectionBuilder::build)();

    return 0;
}
