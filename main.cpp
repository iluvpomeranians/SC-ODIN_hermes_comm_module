#include "core/hs_application.h"

int main() {
    auto* hermes = new hermes::Application();
    hermes->Run();
    delete hermes;
}

