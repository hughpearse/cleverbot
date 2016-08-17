#include <iostream>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <boost/any.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "bot.h"
#include "httplib.h"
#include "logger.h"

using namespace std;

int main(int argc, char* argv[])
{
	try {
		// https://api.ipify.org/?format=json
		boost::asio::io_service io_service;
		client c(io_service, "api.ipify.org", "/?format=txt");
		io_service.run();
		std::cout << c.responseBody << std::endl;
	
		clever_bot::bot bot("irc.freenode.net", "8001");
		//bot.nick("Bot-" + c.responseBody);
		bot.nick("B" + boost::replace_all_copy(c.responseBody, ".", "_"));
		bot.join("#example1");
		
		// Read handlers example (will be improved soon)
		bot.add_read_handler([&bot](const std::string& m) {
			std::istringstream iss(m);
			std::string from, type, to, msg;
			
			iss >> from >> type >> to >> msg;
			
			if (msg == ":!time") {
				//check if private message, and reconfigre "to"
				std::string destNick;
				if(to == bot.nickname){
					destNick = from;
					boost::replace_all(destNick, ":", " ");
					boost::replace_all(destNick, "!", " ");
					std::vector<std::string> msgSenderVect;
					boost::split(msgSenderVect, destNick, boost::is_any_of(" "));
					to = msgSenderVect.at(1);
				}
				
				std::time_t now = std::chrono::system_clock::to_time_t(
					std::chrono::system_clock::now());
				
				bot.message(to, std::ctime(&now));
			}
		});
		
		bot.add_read_handler([&bot](const std::string& m) {
			std::istringstream iss(m);
			std::string from, type, to, msg, text;
			
			iss >> from >> type >> to >> msg;
			
			if (msg == ":!echo") {
				text = "";
				while ((iss >> msg)) {
					text += msg + " ";
				}
				
				if (text != "") {
					bot.message(to, text);
				}
			}
		});
		
		bot.add_read_handler([&bot](const std::string& m) {
			std::istringstream iss(m);
			std::ostringstream oss;
			std::string from, type, to, msg;
			
			iss >> from >> type >> to >> msg;
			
			if (msg == ":!rand") {
				int mx, ans = std::rand();
				
				if (iss >> mx) {
					ans = ans % mx;
				}
				
				oss << ans;
				bot.message(to, from + ": " + oss.str());
			}
		});
		
		// Essential read handler - do not remove!
		// This handler is required to keep the connection alive
		bot.add_read_handler([&bot](const std::string& m) {
			std::istringstream iss(m);
			std::string type, to, text;

			iss >> type;
			if (type == "PING") {
				text = "";
				while ((iss >> to)) {
					text += to + " ";
				}
				bot.pong(text);
			}
		});
		
		// Main execution
		bot.loop();
	}
	catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
	}
	
	return 0;
}
