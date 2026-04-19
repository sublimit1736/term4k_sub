#include "ErrorNotifier.h"

#include <iostream>
#include <utility>

namespace {
    ErrorNotifier::Sink makeDefaultSink() {
        return [](const ErrorNotifier::Level level, const std::string &message) {
            const char* tag = "Info";
            if (level == ErrorNotifier::Level::Warning) tag = "Warning";
            if (level == ErrorNotifier::Level::Error) tag = "Error";
            std::cerr << '[' << tag << "] " << message << '\n';
        };
    }

    ErrorNotifier::Sink &globalSink() {
        static ErrorNotifier::Sink sink = makeDefaultSink();
        return sink;
    }
}

void ErrorNotifier::notify(const std::string &message) {
    globalSink()(Level::Error, message);
}

void ErrorNotifier::notify(const std::string &context, const std::string &message) {
    if (context.empty()){
        notify(message);
        return;
    }
    notify(context + ": " + message);
}

void ErrorNotifier::notifyInfo(const std::string &message) {
    globalSink()(Level::Info, message);
}

void ErrorNotifier::notifyWarning(const std::string &message) {
    globalSink()(Level::Warning, message);
}

void ErrorNotifier::notifyWarning(const std::string &context, const std::string &message) {
    if (context.empty()){
        notifyWarning(message);
        return;
    }
    notifyWarning(context + ": " + message);
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

ErrorNotifier::Sink ErrorNotifier::getSink() {
    return globalSink();
}
