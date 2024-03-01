#include<iostream>
#include "../HTTPConnectionBuilder.h" 

int main() {
    HTTPBuilder connection_builder;

    (connection_builder->*&HTTPConnectionBuilder::add_header)("Content-Type: application/json");
    // Missing add_body call
    auto connection2 = (connection_builder->*&HTTPConnectionBuilder::build)();

    return 0;
}
