#include "ErrorNotifier.h"

#include <iostream>
#include <utility>

namespace {
    ErrorNotifier::Sink makeDefaultSink() {
        return [](const std::string &message) {
            std::cerr << "[Error] " << message << std::endl;
        };
    }

    ErrorNotifier::Sink &globalSink() {
        static ErrorNotifier::Sink sink = makeDefaultSink();
        return sink;
    }
}

void ErrorNotifier::notify(const std::string &message) {
    globalSink()(message);
}

void ErrorNotifier::notify(const std::string &context, const std::string &message) {
    if (context.empty()){
        notify(message);
        return;
    }
    notify(context + ": " + message);
}

void ErrorNotifier::notifyException(const std::string &context, const std::exception &ex) {
    notify(context, ex.what());
}

void ErrorNotifier::notifyUnknown(const std::string &context) {
    notify(context, "unknown exception");
}

void ErrorNotifier::setSink(Sink sink) {
    globalSink() = sink ? std::move(sink) : makeDefaultSink();
}