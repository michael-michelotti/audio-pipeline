#include <iostream>
#include <stdexcept>
#include "audio_server.h"


int main() {
    try {
        AudioServer server;
        if (server.Start(12345)) {
            std::cout << "Server listening on port 12345. Press Enter to stop.\n";
            server.AcceptAndHandle();  // This blocks and handles incoming connections
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
