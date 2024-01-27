#include "TypestateLibrary.h"
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

using TypestateLibrary::TypestateClassConnector;
using TypestateLibrary::State;
using TypestateLibrary::Typestate_Template;

// Dummy connection class
using HTTPConnection = std::tuple<std::vector<std::string>, std::string>;

class HTTPConnectionBuilder {
public:
    HTTPConnectionBuilder() = default;

    void add_header(const std::string& header) {
        headers_.push_back(header);
    }

    void add_body(const std::string& body) {
        body_ = body;
    }

    size_t num_headers() const {
        return headers_.size();
    }

    HTTPConnection build() {
        return HTTPConnection(headers_, body_);
    }

private:
    std::vector<std::string> headers_;
    std::string body_;
};

BETTER_ENUM(HTTPBuilderState, int,
    Start, Headers, Body, Built
);

using HTTPBuilderTypestate = Typestate_Template<
    State<+HTTPBuilderState::Start, &HTTPConnectionBuilder::add_header, +HTTPBuilderState::Headers>,
    State<+HTTPBuilderState::Headers, &HTTPConnectionBuilder::add_header, +HTTPBuilderState::Headers>,
    State<+HTTPBuilderState::Headers, &HTTPConnectionBuilder::add_body, +HTTPBuilderState::Body>,
    State<+HTTPBuilderState::Body, &HTTPConnectionBuilder::build, +HTTPBuilderState::Built>
>;

using HTTPBuilder = TypestateClassConnector<HTTPConnectionBuilder, HTTPBuilderTypestate>;
