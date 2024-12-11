
#include<iostream>
#include "HTTPConnectionBuilder.h" 
int main() {
    HTTPConnectionBuilder connection_builder;
    
    connection_builder.add_header("Content-Type: application/json");
    connection_builder.add_body("{ \"name\": \"John Doe\", \"age\": 30 }");
    connection_builder.build();

    return 0;
}
