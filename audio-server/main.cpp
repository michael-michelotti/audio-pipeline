#include <iostream>
#include <stdexcept>
//#include "audio_server.h"
//#include "audio_playback_server.h"
#include "opus_playback_server.h"


int main() {
    try {
        OpusPlaybackServer server;
        if (server.Start(12345)) {
            std::cout << "Server listening on port 12345. Press Enter to stop.\n";

            // Start server handling in a separate thread
            std::thread serverThread([&server]() {
                server.AcceptAndHandle();
                });

            // Wait for Enter key
            std::cin.get();

            // Stop the server
            server.Stop();

            // Wait for server thread to finish
            if (serverThread.joinable()) {
                serverThread.join();
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
