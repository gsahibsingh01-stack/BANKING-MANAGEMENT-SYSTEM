#include <iostream>
#include <string>
#include <csignal>
#include <filesystem>
#include "../../dependencies/httplib.h"
#include "../include/database.h"
#include "../include/controllers.h"

namespace fs = std::filesystem;

httplib::Server* g_svr = nullptr;

void signalHandler(int signum) {
    std::cout << "\n[Server] Intercepted signal " << signum << ". Stopping server gracefully..." << std::endl;
    if (g_svr) {
        g_svr->stop();
    }
}

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "Invalid port specified. Using default: 8080" << std::endl;
        }
    }

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "======================================================================\n";
    std::cout << "              APEX NATIONAL BANK - MANAGEMENT SYSTEM                  \n";
    std::cout << "         Full-Stack Banking Enterprise Application (C++ Backend)      \n";
    std::cout << "======================================================================\n";

    // Ensure database directory exists
    if (!fs::exists("database")) {
        fs::create_directories("database");
    }

    std::string dbPath = "database/bank.db";
    std::string schemaPath = "database/schema.sql";
    std::string seedPath = "database/seed.sql";
    std::string frontendPath = "frontend";

    // Adjust paths if running from backend/ or build/
    if (!fs::exists(schemaPath) && fs::exists("../database/schema.sql")) {
        dbPath = "../database/bank.db";
        schemaPath = "../database/schema.sql";
        seedPath = "../database/seed.sql";
        frontendPath = "../frontend";
    }

    Database db;
    if (!db.open(dbPath)) {
        std::cerr << "[Fatal] Failed to connect to SQLite database at: " << dbPath << std::endl;
        return 1;
    }

    if (!db.initSchemaAndSeed(schemaPath, seedPath)) {
        std::cerr << "[Warning] Schema or seed check encountered an issue." << std::endl;
    }

    httplib::Server svr;
    g_svr = &svr;

    // Register all API routes
    Controllers::registerRoutes(svr, db);

    // Serve Frontend Static Files
    if (fs::exists(frontendPath)) {
        svr.set_mount_point("/", frontendPath);
        std::cout << "[Server] Serving frontend assets from: " << fs::absolute(frontendPath).string() << std::endl;
    } else {
        std::cerr << "[Warning] Frontend directory not found at: " << frontendPath << std::endl;
    }

    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_redirect("/index.html");
    });

    svr.set_error_handler([](const httplib::Request& /*req*/, httplib::Response& res) {
        if (res.status == 404 && res.body.empty()) {
            res.set_content(
                "<!DOCTYPE html><html><head><title>404 Not Found</title></head>"
                "<body style='font-family:sans-serif;text-align:center;padding:50px;background:#0f172a;color:#f8fafc;'>"
                "<h1>404 - Resource Not Found</h1><p>Apex Banking API Server</p>"
                "<a href='/' style='color:#38bdf8;'>Back to Online Banking</a></body></html>",
                "text/html"
            );
        }
    });

    std::cout << "----------------------------------------------------------------------\n";
    std::cout << " [✓] C++ Server successfully started!\n";
    std::cout << " [✓] Web Application URL: http://localhost:" << port << "\n";
    std::cout << " [✓] REST API Base URL:  http://localhost:" << port << "/api\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << " Default Demo Accounts for Evaluation / Presentation:\n";
    std::cout << "  • Customer Portal:  username: 'customer' | password: 'customer123'\n";
    std::cout << "  • Customer 2:       username: 'alice'    | password: 'alice123'\n";
    std::cout << "  • Branch Manager:   username: 'manager'  | password: 'manager123'\n";
    std::cout << "  • Administrator:    username: 'admin'    | password: 'admin123'\n";
    std::cout << "======================================================================\n";
    std::cout << "Ready and listening on 0.0.0.0:" << port << " (Press Ctrl+C to stop)...\n";

    svr.listen("0.0.0.0", port);

    db.close();
    std::cout << "[Server] Server stopped cleanly. Good-bye!" << std::endl;
    return 0;
}
