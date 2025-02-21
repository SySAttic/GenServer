#include "includes.h"
#include "managers.h"
#include "ChannelInfo.h"
#include "clan.h"
#include "userinfo.h"
#include "Apgar.h"
#include "resource.h"
#include "wdtmapstate.h"
std::mutex fileMutex;


bool checkAndCreateFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(fileMutex);
    std::ifstream infile(filename);
    if (!infile) {
        std::ofstream outfile(filename); // Create the file if it does not exist
        if (!outfile) {
            std::cerr << "Error creating " << filename << std::endl;
            return false;
        }
    }
    return true;
}

void readBannedUsers() {
    const std::string filename = "banned_users.txt";
    if (!checkAndCreateFile(filename)) {
        return;
    }
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Error opening " << filename << " for reading" << std::endl;
        return;
    }
    std::string line;
    while (std::getline(infile, line)) {
        // Process each line (e.g., add to a banned users list)
    }
    infile.close();
}



struct AnongameWolPlayer {
    std::string conn;
    int address;
    int port;
    int country;
    int colour;

    AnongameWolPlayer() : conn(""), address(0), port(0), country(-2), colour(-2) {} // Default constructor
    AnongameWolPlayer(const std::string& conn) : conn(conn), address(0), port(0), country(-2), colour(-2) {}
};
// Define tag handling function type
typedef int (*TagHandler)(AnongameWolPlayer* player, const std::string& param);

// Define tag table row structure
struct WolAnongameTagTableRow {
    std::string tag;
    TagHandler handler;
};

// Function prototypes for tag handlers
int handleAddressTag(AnongameWolPlayer* player, const std::string& param);
int handlePortTag(AnongameWolPlayer* player, const std::string& param);
int handleCountryTag(AnongameWolPlayer* player, const std::string& param);
int handleColourTag(AnongameWolPlayer* player, const std::string& param);

// Tag table
static const WolAnongameTagTableRow tagTable[] = {
    { "ADDRESS", handleAddressTag },
    { "PORT", handlePortTag },
    { "COUNTRY", handleCountryTag },
    { "COLOUR", handleColourTag },
    { "", nullptr }
};

// Player management
std::unordered_map<std::string, AnongameWolPlayer> players;
std::mutex playersMutex;

AnongameWolPlayer* createPlayer(const std::string& conn) {
    std::lock_guard<std::mutex> lock(playersMutex);
    players[conn] = AnongameWolPlayer(conn);
    return &players[conn];
}

int destroyPlayer(const std::string& conn) {
    std::lock_guard<std::mutex> lock(playersMutex);
    auto it = players.find(conn);
    if (it != players.end()) {
        players.erase(it);
        return 0;
    }
    return -1;
}

AnongameWolPlayer* getPlayer(const std::string& conn) {
    std::lock_guard<std::mutex> lock(playersMutex);
    auto it = players.find(conn);
    if (it != players.end()) {
        return &it->second;
    }
    return nullptr;
}

// Tag handling functions
int handleAddressTag(AnongameWolPlayer* player, const std::string& param) {
    player->address = std::stoi(param);
    return 0;
}

int handlePortTag(AnongameWolPlayer* player, const std::string& param) {
    player->port = std::stoi(param);
    return 0;
}

int handleCountryTag(AnongameWolPlayer* player, const std::string& param) {
    player->country = std::stoi(param);
    return 0;
}

int handleColourTag(AnongameWolPlayer* player, const std::string& param) {
    player->colour = std::stoi(param);
    return 0;
}

int setPlayerSetting(AnongameWolPlayer* player, const std::string& tag, const std::string& param) {
    for (const auto& row : tagTable) {
        if (row.tag == tag) {
            if (row.handler) {
                return row.handler(player, param);
            }
        }
    }
    return -1;
}

// Tokenize line and set player settings
int tokenizeLine(const std::string& conn, const std::string& text) {
    AnongameWolPlayer* player = createPlayer(conn);

    std::istringstream iss(text);
    std::string command;
    iss >> command;

    if (command == "Match") {
        std::string token;
        while (std::getline(iss, token, ',')) {
            size_t pos = token.find('=');
            if (pos != std::string::npos) {
                std::string tag = token.substr(0, pos);
                std::string param = token.substr(pos + 1);
                setPlayerSetting(player, tag, param);
            }
        }
    }
    return 0;
}


HINSTANCE hInst;
HWND hwndMain;
HWND hwndEdit;
SOCKET listenSocket;
std::thread serverThread;
bool serverRunning = true;
std::set<std::string> bannedUsers;
void AppendTextToEditControl(HWND hwndEdit, const std::string& text) {
    int len = GetWindowTextLength(hwndEdit);
    SendMessage(hwndEdit, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessage(hwndEdit, EM_REPLACESEL, 0, (LPARAM)text.c_str());
}


class ServerUserClient {
private:
    SOCKET socket;
    UserList& userlist; // Use reference
    std::string ip;
    ChannelList& channellist; // Use reference
    std::string serverName;
    std::string serverIP;
    std::string serverPort;
    std::string serverVersion;
    std::string nick;
    std::string pass;
    std::string ip_nr;
    std::string codepage;
    std::string channel;
    std::string g_maxplayers;
    int clientVersion;
    bool endThread = false;
    bool ping_active = true;
    bool first_joingame = true;



    int getUsersCount() {
        int totalUsers = 0;
        for (const auto& channel : channellist.getChannelList()) {
            totalUsers += userlist.countUsersInChannel(channel.name);
        }
        return totalUsers;
    }

    void initializePermanentChannels() {
        if (clientVersion == 0) {
            for (int i = 0; i < 3; i++) {
                char channelName[20];
                channellist.addChannel(User(), "#Command_n_Conqr", "500");
                channellist.addChannel(User(), "#Monopoly", "500");
                channellist.addChannel(User(), "#Red_Alert", "500");
                channellist.addChannel(User(), "#Lob_19_0", "500");
                channellist.addChannel(User(), "#Lob_38_0", "500");
                channellist.addChannel(User(), "#Lob_39_0", "500");
                channellist.addChannel(User(), "#Lob_40_0", "500");
                channellist.addChannel(User(), "#Lob_23_2", "500");
                channellist.addChannel(User(), "#Lob_24_0", "500");
                channellist.addChannel(User(), "#Lob_31_0", "500");
                channellist.addChannel(User(), "#Lob_33_0", "500");
                channellist.addChannel(User(), "#Lob_37_0", "500");
                channellist.addChannel(User(), "#Lob_41_0", "500");
                sprintf_s(channelName, "#Lob_18_%d", i);
                //channellist.addChannel(User(),"@matchbot,0,0","500");
                channellist.addChannel(User(), channelName, "500");
            }
        }
    }

public:
    ServerUserClient(SOCKET sock, UserList& ul, ChannelList& cl, const std::string& srvName, const std::string& srvIP, const std::string& srvPort, const std::string& srvVersion)
        : socket(sock), userlist(ul), channellist(cl), serverName(srvName), serverIP(srvIP), serverPort(srvPort), serverVersion(srvVersion), clientVersion(0) {
        initializePermanentChannels();

        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(addr);
        getpeername(sock, (struct sockaddr*)&addr, &addr_len);
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        ip = std::string(client_ip);
    }

    std::string getNick() const {
        return nick;
    }


    void handleStartGCommandCustom(const std::string& userNick, const std::string& userIP) {
        std::string response = userNick + "!" + nick + "@" + serverName + " STARTG Username : " + userNick + " " + userIP + " SunBot01 127.0.0.1 : 12 1115237380\r\n";
        send(response);
    }

    void run() {
        std::vector<char> buffer(1024);
        int bytesReceived;
        std::string commandBuffer;

        while ((bytesReceived = recv(socket, buffer.data(), buffer.size() - 1, 0)) > 0) {
            buffer[bytesReceived] = '\0'; // Ensure null-termination
            commandBuffer += std::string(buffer.data()); // Append to the command buffer

            // Process the command buffer to handle complete commands
            size_t pos;
            while ((pos = commandBuffer.find("\r\n")) != std::string::npos) {
                std::string command = commandBuffer.substr(0, pos);
                commandBuffer.erase(0, pos + 2); // Remove the processed command

                // Log the received command
                std::cout << "<-: " << command << std::endl;
                AppendTextToEditControl(hwndEdit, "<-: " + command + "\r\n"); // Append to log window

                // Tokenize the command
                std::istringstream iss(command);
                std::string token;
                std::vector<std::string> tokens;

                // Use getline with space delimiter to avoid splitting usernames incorrectly
                while (std::getline(iss, token, ' ')) {
                    // Trim and add non-empty tokens
                    token = trim(token);
                    if (!token.empty()) {
                        tokens.push_back(token);
                    }
                }

                if (tokens.empty()) continue;
                std::cout << "Recognized command: " << tokens[0] << std::endl;
                AppendTextToEditControl(hwndEdit, "Recognized command: " + tokens[0] + "\r\n"); // Append to log window

                // Handle the command

                 if (tokens[0] == "LOBCOUNT") {
                    handleLobCountCommand(tokens);
                }
                else if (tokens[0] == "NICK") {
                    handleNickCommand(tokens);
                }
                else if (tokens[0] == "SETLOCALE") {
                    handleSetLocaleCommand(tokens);
                }
                else if (tokens[0] == "GETTEAM") {
                    handleGetTeamCommand(tokens);
                }
                else if (tokens[0] == "SETTEAM") {
                    handleGetTeamCommand(tokens);
                }
                else if (tokens[0] == "TEAMSELECT") {
                    handleGetTeamCommand(tokens);
                }
                else if (tokens[0] == "REGION") {
                    handleGetTeamCommand(tokens);
                }
                else if (tokens[0] == "REGN") {
                    handleGetTeamCommand(tokens);
                }
                else if (tokens[0] == "STATE") {
                    handleGetTeamCommand(tokens);
                }
                else if (tokens[0] == "TEAMADD") {
                    handleGetTeamCommand(tokens);
                }
                else if (tokens[0] == "TEAMCREATE") {
                    handleGetTeamCommand(tokens);
                }
                else if (tokens[0] == "WDTSTATE") {
                    handleGetTeamCommand(tokens);
                }
                else if (tokens[0] == "WDT") {
                    handleGetTeamCommand(tokens);
                }
                else if (tokens[0] == "USERTEAM") {
                    handleGetTeamCommand(tokens);
                }
                else if (tokens[0] == "PASS") {
                    handlePassCommand(tokens);
                }
                else if (tokens[0] == "USER") {
                    handleUserCommand(tokens);
                }
                else if (tokens[0] == "NAMES") {
                    handleNamesCommand(tokens);
                }
                else if (tokens[0] == "MOTD") {
                    handleMotdCommand();
                }
                else if (tokens[0] == "VERCHK") {
                    handleVerChkCommand(tokens);
                }
                else if (tokens[0] == "SETCODEPAGE") {
                    handleSetCodePageCommand(tokens);
                }
                else if (tokens[0] == "SETOPT") {
                    handleSetOptCommand(tokens);
                }
                else if (tokens[0] == "LIST") {
                    handleListCommand(tokens);
                }
                else if (tokens[0] == "ADVERTR") {
                    handleAdvertrCommand(tokens);
                }
                else if (tokens[0] == "JOIN") {
                    handleJoinCommand(tokens);

                }
                else if (tokens[0] == "MODE") {
                    handleModeCommand(tokens);
                }
                else if (tokens[0] == "CVERS") {
                    handlecversCommand(tokens);
                }
                else if (tokens[0] == "JOINGAME") {
                    handleJoinGameCommand(tokens);
                }
                else if (tokens[0] == "PART") {
                    handlePartCommand(tokens);
                }
                else if (tokens[0] == "GETCODEPAGE") {
                    handleGetCodePageCommand(tokens);
                    handleNamesCommand({ "NAMES", channel });
                }
                else if (tokens[0] == "GAMEOPT") {
                    handleGameOptCommand(tokens);
                }
                else if (tokens[0] == "PRIVMSG") {
                    handlePrivMsgCommand(tokens);
                 //   handleMatchbotCommand(*this, tokens);
                }
                else if (tokens[0] == "PAGE") {
                    handlePageCommand(tokens);
                }
                else if (tokens[0] == "GETLOCALE") {
                    handleGetLocaleCommand(tokens);
                }
                else if (tokens[0] == "FINDUSEREX") {
                    handleFindUserExCommand(tokens);
                }
                else if (tokens[0] == "FINDUSER") {
                    handleFindUserCommand(tokens);
                }
                else if (tokens[0] == "TOPIC") {
                    handleTopicCommand(tokens);
                }
                else if (tokens[0] == "STARTG") {
                    handleStartGCommand(tokens);
                }
                else if (tokens[0] == "CHANCHK") {
                    handleChanChkCommand(tokens);
                }
                else if (tokens[0] == "SQUADINFO") {
                    handleSquadInfoCommand(tokens);
                }
                else if (tokens[0] == "GETBUDDY") {
                    handleGetBuddyCommand(tokens);
                }
                else if (tokens[0] == "ADDBUDDY") {
                    handleAddBuddyCommand(tokens);
                }
                else if (tokens[0] == "DELBUDDY") {
                    handleDelBuddyCommand(tokens);
                }
                else if (tokens[0] == "QUIT") {
                    handleQuitCommand(tokens);
                }
                else if (tokens[0] == "PING") {
                    handlePingCommand(tokens);
                }
                else if (tokens[0] == "apgar") {
                    handleApgarCommand(tokens);
                }
                else if (tokens[0] == "APGAR") {
                    handleApgarCommand(tokens);
                }
                else if (tokens[0] == "KICK") {
                    handleKickCommand(tokens);
                }
                else if (tokens[0] == "BAN") {
                    handleBanCommand(tokens);
                }
                else {
                    std::cout << "Unrecognized command: " << tokens[0] << std::endl;
                    AppendTextToEditControl(hwndEdit, "Unrecognized command: " + tokens[0] + "\r\n"); // Append to log window
                }
            }
        }
        // Handle unexpected disconnection
        if (bytesReceived == 0 || bytesReceived == SOCKET_ERROR) {
            std::cerr << "Client disconnected unexpectedly" << std::endl;
            AppendTextToEditControl(hwndEdit, "Client disconnected unexpectedly\r\n"); // Append to log window
            handleQuitCommand({ "QUIT" });
        }

        closesocket(socket);
    }

    void RejectClient(const std::string& message) {
        send("ERROR: " + message);
        closesocket(socket);
    }

    void createChannel(const std::string& channelName, const User& creator) {
        std::lock_guard<std::mutex> lock(channellistMutex);

        if (!channellist.isExistChannel(channelName)) {
            // Add the channel to the list with the creator as the operator
            channellist.addChannel(creator, channelName);

            // Add the creator as an operator
            channelOperators[channelName].insert(creator.nick);

            std::cout << "Channel " << channelName << " created by " << creator.nick << std::endl;
        }
        else {
            std::cerr << "Channel " << channelName << " already exists." << std::endl;
        }
    }



    void handleMotdCommand() {
        send(":" + serverName + " 001 " + nick + " :Welcome to Westwood Online!");
        send(":" + serverName + " 375 " + nick + " :- " + serverName + " Message of the Day -");

        send(":" + serverName + " 372 " + nick + " :- Running AWOS");
        send(":" + serverName + " 372 " + nick + " :- Local Date and time: " + getCurrentDateTime());
        send(":" + serverName + " 372 " + nick + " :- Started At: " + getStartTime());
        send(":" + serverName + " 372 " + nick + " :- ");

        std::vector<std::string> motdLines = splitMotd(Config.MOTD);
        for (const auto& line : motdLines) {
            send(":" + serverName + " 372 " + nick + " :- " + line);
        }

        send(":" + serverName + " 372 " + nick + " :- *** This is not an official Westwood Server ***");
        send(":" + serverName + " 372 " + nick + " :- Known Users   : " + std::to_string(getUsersCount()));
        send(":" + serverName + " 372 " + nick + " :- Connections   : " + std::to_string(SocketTotal));

        send(":" + serverName + " 376 " + nick + " :End of /MOTD command.");
        //  handleSpecificListCommand();
    }

    void handleGetTeamCommand(const std::vector<std::string>& token) {


        // Assign different values to the response variable and send it

    


    }

    void handleMatchbotCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 3) {
            send("Not enough parameters for matchbot command");
            return;
        }

        const std::string& command = tokens[2];
		std::cout << "Matchbot command: " << command << std::endl;
        if (command == "Match") {
            // Example: :user!EMPR@host PRIVMSG matchbot :Match COU=-1,COL=-1,SHA=-1,SHB=-1,LOC=2,RAT=0
            std::string params = tokens[3];
            tokenizeLine(getNick(), params);
            AnongameWolPlayer* player = getPlayer(getNick());
            if (player) {
                std::cout << "Player address: " << player->address << std::endl;
                std::cout << "Player port: " << player->port << std::endl;
                std::cout << "Player country: " << player->country << std::endl;
                std::cout << "Player colour: " << player->colour << std::endl;
            }
            // Process Match command params
            send(params);
        }

   
        else if (command == "CINFO") {
            // Example: :user!RNGD@host PRIVMSG matchbot :CINFO VER=1329937315 CPU=2981 MEM=1023 TPOINTS=0 PLAYED=0 PINGS=00FFFFFFFFFFFFFF
            std::map<std::string, std::string> params;
            std::istringstream ss(tokens[3]);
            std::string param;
            while (std::getline(ss, param, ' ')) {
                auto pos = param.find('=');
                if (pos != std::string::npos) {
                    std::string key = param.substr(0, pos);
                    std::string value = param.substr(pos + 1);
                    params[key] = value;
                }
            }
            // Process CINFO command params
            send(tokens[3]);
        }
        else if (command == "SINFO") {
            // Example: :user!RNGD@host PRIVMSG matchbot :SINFO 4F453BA3DBAE41CB00000000000000002d# 9Dedicated Renegade Server-C&C_Field.mix07FF0656FFFF1C2D|090000000000 
            send(tokens[3]);
        }
        else if (command == "Pings") {
            // Example: :user!YURI@host PRIVMSG matchbot :Pings nickname,2;
            send(tokens[3]);
        }
        else {
            send("Unknown matchbot command: " + command);
        }
    }

    void handleLobCountCommand(const std::vector<std::string>& tokens) {

        if (tokens.size() < 2) {
            send(":" + serverName + " 461 " + nick + " " + tokens[0] + " :Not enough parameters");
            return;
        }

        std::string whichLobby = tokens[1];
        send(":" + serverName + " 610 " + nick + " 1");
    }


    bool isUserOperator(const std::string& channelName, const std::string& nick) {
        auto it = channelOperators.find(channelName);
        if (it != channelOperators.end()) {
            return it->second.find(nick) != it->second.end();
        }
        return false;
    }
    void handleNamesCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) {
            send(":" + serverName + " 461 " + nick + " " + tokens[0] + " :Not enough parameters");
            return;
        }

        std::string channelName = tokens[1];

        std::lock_guard<std::mutex> lock(channellistMutex); // Lock the channel list mutex

        if (!channellist.isExistChannel(channelName)) {
            send(":" + serverName + " 403 " + nick + " " + channelName + " :No such channel");
            send(":" + serverName + " 366 " + nick + " " + channelName + " :End of /NAMES list.");
            return;
        }

        std::lock_guard<std::mutex> userLock(userlistMutex); // Lock the user list mutex

        auto usersInChannel = userlist.getUsersInChannel(channelName);

        if (usersInChannel.empty()) {
            send(":" + serverName + " 366 " + nick + " " + channelName + " :No users in channel.");
            return;
        }

        std::string namesList;
        for (const auto& user : usersInChannel) {
            std::string userPrefix;
            if (isUserOperator(channelName, user.nick)) {
                userPrefix = "@";
            }
            else {
                userPrefix = "+"; // Normal user
            }
            // Assuming User class has 'info.Username', 'Userdata.Clan', and 'IPInt' attributes
            namesList += userPrefix + user.nick + "," + "test" + "," + ip + " ";
        }

        // Send the user list to the requesting user
        send(":" + serverName + " 353 " + nick + " * " + channelName + " :" + namesList);
        send(":" + serverName + " 366 " + nick + " " + channelName + " :End of /NAMES list.");
    }


    // Function to check if a nickname is already stored in the apgar_responses.txt file
    bool isNickInApgarResponses(const std::string& nick) {
        std::ifstream infile("apgar_responses.txt");
        if (!infile.is_open()) {
            std::cerr << "Error opening apgar_responses.txt" << std::endl;
            return false;
        }

        std::string line;
        while (std::getline(infile, line)) {
            std::istringstream iss(line);
            std::string storedNick;
            if (std::getline(iss, storedNick, ':')) {
                storedNick = trim(storedNick);
                if (storedNick == nick) {
                    return true;
                }
            }
        }

        return false;
    }
    // Function to handle APGAR command
    void handleApgarCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) {
            send(":" + serverName + " 461 " + nick + " " + tokens[0] + " :Not enough parameters");
            return;
        }

        std::string apgarValue = tokens[1];

        // Check if the APGAR value is already stored
        if (checkApgarResponse(nick, apgarValue)) {
            send(":" + serverName + " 200 " + nick + " :APGAR value already recorded");
            return;
        }

        // Ensure the nickname is not already in the file with a different APGAR token
        if (isNickInApgarResponses(nick)) {
            send(":" + serverName + " 433 * " + nick + " :Nickname already has a different APGAR value stored");
            RejectClient("Nickname already has a different APGAR value stored\r\n");
            std::cout << "User with nick " << nick << " was disconnected for different APGAR value, IP: " << ip << std::endl;
            return;
        }

        // Store the APGAR value if it doesn't exist
        std::ofstream outfile("apgar_responses.txt", std::ios_base::app); // Append to the file
        if (outfile.is_open()) {
            outfile << nick << ": " << apgarValue << std::endl; // Write the nickname and APGAR value to the file
            outfile.close();
        }
        else {
            send(":" + serverName + " 500 " + nick + " :File writing error");
        }

        send(":" + serverName + " 200 " + nick + " :APGAR value recorded");
    }

    void handleNickCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) {
            send(":" + serverName + " 431 * :No nickname given\r\n");
            return;
        }
        send(":" + serverName + " 465 * :Serial banned\r\n");

        std::string newNick = tokens[1];
        std::string oldNick = nick;

        if (oldNick.empty()) {
            oldNick = "*";
        }

        if (!isValidNick(newNick)) {
            send(":" + serverName + " 432 * " + newNick + " :Erroneous nickname\r\n");
            RejectClient("Erroneous nickname reconnect:\r\n");
            std::cout << "User with nick " << newNick << " was disconnected for erroneous name " << newNick << ", IP: " << ip << std::endl;
            return;
        }

        // Check if the user is reconnecting with the correct APGAR value
        if (userlist.isUserExist(newNick)) {
            std::string apgarValue = "..."; // Retrieve the APGAR value from the client (this is just a placeholder)
            if (!checkApgarResponse(newNick, apgarValue)) {
                send(":" + serverName + " 378 * UserName :Error: incorrect password\r\n");
                send(":" + serverName + " 375 * UserName :- Welcome to AWOS!\r\n");
                send(":" + serverName + " 372 UserName :- Closing Connection. incorrect password\r\n");
                send(":" + serverName + " 376 UserName :/END of motd command\r\n");
                send(":" + serverName + " 433 * " + nick + " :Nickname already has a different APGAR value stored");

                RejectClient("APGAR value mismatch\r\n");
                std::cout << "User with nick " << newNick << " was disconnected for APGAR value mismatch, IP: " << ip << std::endl;
                return;
            }
        }

        // Check if the nickname is already in use by another user
        if (userlist.isUserExist(newNick) && newNick != oldNick) {
            send(":" + serverName + " 433 * " + newNick + " :Nickname is already in use\r\n");
            return;
        }

        nick = newNick;

        User user(newNick, ip);
        userlist.addUser(user, newNick);

        nicknameCooldown[newNick] = std::chrono::steady_clock::now();

        std::cout << "User's Nick changed from " << oldNick << " to " << newNick << ", IP: " << ip << std::endl;

        if (oldNick != "*") {
            std::string changeNickMessage = ":" + oldNick + " NICK :" + newNick + "\r\n";
            send(changeNickMessage);
            send_channel(userlist.getChannelName(oldNick), changeNickMessage);
        }
    }






    bool isValidNick(const std::string& nick) {
        for (char c : nick) {
            if (!isalnum(c) && c != '_' && c != '-') {
                return false;
            }
        }
        return true;
    }

    bool isNickCollision(const std::string& nick) {
        return userlist.isUserExist(nick);
    }

    bool isNickTemporarilyUnavailable(const std::string& nick) {
        auto it = nicknameCooldown.find(nick);
        if (it != nicknameCooldown.end()) {
            auto now = std::chrono::steady_clock::now();
            if (now - it->second < cooldownPeriod) {
                return true;
            }
        }
        return false;
    }

    void handlePassCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() != 2) {
            RejectClient("Invalid Parameter(s)");
            return;
        }

        std::string providedPass = tokens[1];
        if (providedPass != "supersecret") {
            RejectClient("Invalid Parameter(s)");
            return;
        }

        // Set a flag to indicate the password was given
        Info.GavePass = true;
    }


    void handleUserCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 5) {
            send(":" + serverName + " 461 " + nick + " " + tokens[0] + " :Not enough parameters");
            return;
        }

        std::string userName = tokens[1];
        std::string hostName = tokens[2];
        std::string serverName = tokens[3];
        std::string realName = tokens[4];


        User user(userName, ip);
        userlist.addUser(user, userName);
        std::cout << "User " << userName << " added to user list with IP: " << ip << std::endl;

        // Send a welcome message to the user
        send(":" + serverName + " 001 " + userName + " :AWOS " + userName);
        send(":" + serverName + " 002 " + userName + " :Your host is " + serverName);
        send(":" + serverName + " 003 " + userName + " :This server was created on " + getStartTime());
        send(":" + serverName + " 004 " + userName + " " + serverName + " 1.0 iowghraAsORTVSxNCWqBzvdHtGpblmE");

        // Handle MOTD (Message of the Day)
        handleMotdCommand();
    }

    void handleVerChkCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) return;
        send(": 379 " + nick + " :none none none 1 " + tokens[1] + " NONREQ");
    }

    void handleSetCodePageCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() == 2) {
            // Assuming 'nick' is the current user's nickname
            if (userlist.isUserExist(nick)) {
                User user = userlist.getUser(nick);
                user.codepage = tokens[1];  // Update the user's codepage
                userlist.addUser(userlist.getUser(nick), nick);
                send(":" + serverName + " 329 " + nick + " " + tokens[1]);
            }
        }
        else {
            if (userlist.isUserExist(nick)) {
                User user = userlist.getUser(nick);
                send(":" + serverName + " 329 " + nick + " " + user.codepage);
            }
        }
    }

    void handleSetLocaleCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) {
            send(":" + serverName + " 461 " + nick + " " + tokens[0] + " :Not enough parameters");
            return;
        }

        char response[512];
        char cmd[512];

        // Clean up
        Sleep(3000);
        std::this_thread::sleep_for(std::chrono::seconds(3));

        std::strcpy(response, ":irc.westwood.com 307 test 1\n");
        send(response);
        std::strcpy(response, ":irc.westwood.com 308 test 1\n");
        send(response);

        // Broadcast actual values from WDTState
        send("TOTK 1");
        send("CYLN 1");
        send("WURL westwood.com");
        send("SDSC European Campaign");
        send("LDSC European Campaign");

        for (int i = 1; i < 30; i++) {
            std::memset(cmd, 0, sizeof(cmd));
            std::strcat(cmd, "hsvc ");
            std::strcat(cmd, std::to_string(i).c_str());
            std::strcat(cmd, " 1 1 29803,7,6,5000,0,0,1,1,2,0,0,0-100-2,0-100-100,33,18,0,0-100-64,9,0x11ae1f8,0xb98\r\n");
            send(cmd);

            std::strcpy(response, ":irc.westwood.com : 611 UserName :irc.westwood.com 4010 'WDT Server' -8 36.1083 -115.0582\n");
            send(response);

            std::strcpy(response, ":irc.westwood.com 307 test 1\n");
            send(response);
            std::strcpy(response, ":irc.westwood.com 308 test 1\n");
            send(response);
        }

        send("CMID 1");
        send("CYID 1");
        send("TICC 1");
        //RejectClient("Invalid Parameter(s)");   

        for (size_t i = 1; i < tokens.size(); ++i) {
            const std::string& userNick = tokens[i];
            if (userlist.isUserExist(userNick)) {
                User user = userlist.getUser(userNick);
                // Assuming User class has a 'locale' attribute
                send(":" + serverName + " 309 " + nick + " " + userNick + "`" + std::to_string(user.locale));
            }
            else {
                send(":" + serverName + " 309 " + nick + " " + userNick + " :User not found");
            }
        }
    }


    void handleKickCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 3) {
            send(":" + serverName + " 461 " + nick + " KICK :Not enough parameters");
            return;
        }

        std::string channelName = tokens[1];
        std::string targetNick = tokens[2];

        {
            std::lock_guard<std::mutex> lock(channellistMutex);
            if (!channellist.isExistChannel(channelName)) {
                send(":" + serverName + " 403 " + nick + " " + channelName + " :No such channel");
                return;
            }
        }

        {
            std::lock_guard<std::mutex> lock(userlistMutex);
            if (!userlist.isUserInChannel(targetNick, channelName)) {
                send(":" + serverName + " 441 " + nick + " " + targetNick + " " + channelName + " :They aren't on that channel");
                return;
            }

            userlist.removeUserFromChannel(targetNick, channelName);
        }

        // Notify all users in the channel about the kick
        std::string kickMessage = ":" + nick + " KICK " + channelName + " " + targetNick + " :Kicked by " + nick + "\r\n";
        send_channel(channelName, kickMessage);

    }


    bool isUserBannedFromChannel(const std::string& nick, const std::string& channelName) {
        std::lock_guard<std::mutex> lock(channellistMutex);
        if (!channellist.isExistChannel(channelName)) {
            return false;
        }

        Channel* channel = channellist.getChannel(channelName);
        return std::find(channel->bans.begin(), channel->bans.end(), nick) != channel->bans.end();
    }



    void handleBanCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 3) {
            send(":" + serverName + " 461 " + nick + " BAN :Not enough parameters");
            return;
        }

        std::string channelName = tokens[1];
        std::string targetNick = tokens[2];

        std::lock_guard<std::mutex> lock(channellistMutex);
        if (!channellist.isExistChannel(channelName)) {
            send(":" + serverName + " 403 " + nick + " " + channelName + " :No such channel");
            return;
        }

        Channel* channel = channellist.getChannel(channelName);
        if (std::find(channel->bans.begin(), channel->bans.end(), targetNick) == channel->bans.end()) {
            channel->bans.push_back(targetNick);
            send(":" + serverName + " MODE " + channelName + " +b " + targetNick + "\r\n");
            std::ofstream banFile("banned_users.txt", std::ios_base::app);
            if (banFile.is_open()) {
                banFile << targetNick << " " << channelName << std::endl;
                banFile.close();
            }
            else {
				send(":" + serverName + " 500 " + nick + " :File writing error");
            }
        }
        else {
            send(":" + serverName + " 482 " + nick + " " + channelName + " :User is already banned");
        }

        // Optionally kick the user if they are currently in the channel
        if (userlist.isUserInChannel(targetNick, channelName)) {
            handleKickCommand({ "KICK", channelName, targetNick });
        }
    }


    void handleGetLocaleCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) {
            send(":" + serverName + " 461 " + nick + " " + tokens[0] + " :Not enough parameters");
            return;
        }

        std::string response;

        for (size_t i = 1; i < tokens.size(); ++i) {
            const std::string& userNick = tokens[i];
            int locale = 0;
            AnongameWolPlayer* player = getPlayer(userNick);
            if (player) {
                locale = player->country;  // Assuming country represents locale
            }

            response += userNick + "`" + std::to_string(locale);
            if (i < tokens.size() - 1) {
                response += "`";
            }
        }
        send(":" + serverName + " 309 " + nick + " " + response);
    }



    void handleSetOptCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) {
            send(":" + serverName + " 461 " + nick + " " + tokens[0] + " :Not enough parameters");
            return;
        }

        std::vector<std::string> options;
        std::istringstream iss(tokens[1]);
        std::string option;
        while (std::getline(iss, option, ',')) {
            options.push_back(option);
        }

        if (options.size() != 2) {
            send(":" + serverName + " 461 " + nick + " " + tokens[0] + " :Invalid number of options");
            return;
        }

        try {
            int findOption = std::stoi(options[0]);
            int pageOption = std::stoi(options[1]);

            // Store options in a map
            std::unordered_map<std::string, int> userOptions;
            userOptions[nick + "_find"] = findOption;
            userOptions[nick + "_page"] = pageOption;

            // Acknowledge the options have been set
            send(":" + serverName + " 250 " + nick + " :Options set successfully");

        }
        catch (const std::invalid_argument& e) {
            send(":" + serverName + " 461 " + nick + " " + tokens[0] + " :Invalid option format");
        }
        catch (const std::out_of_range& e) {
            send(":" + serverName + " 461 " + nick + " " + tokens[0] + " :Option out of range");
        }
    }
    void handleListCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() >= 3 && tokens[1] == "0" && tokens[2] == "18") {
            // Handle the specific LIST 0 18 command
            send(":" + serverName + " 321 " + nick + " Channel :Users Names\r\n");

            auto channels = channellist.getChannelList();
            for (const auto& channel : channels) {
                size_t userCount = userlist.countUsersInChannel(channel.name);
                std::string topic = channel.topic.empty() ? "No topic" : channel.topic;
                send(":" + serverName + " 327 " + nick + " " + channel.name + " " + std::to_string(userCount) + " 1 388 :" + topic + "\r\n");

                // Call handleNamesCommand to list users in the channel
                handleNamesCommand({ "NAMES", channel.name });
            }

            send(":" + serverName + " 323 " + nick + " :End of /LIST command\r\n");
        }
        else if (tokens.size() >= 3 && tokens[1] == "18" && tokens[2] == "18") {
            // Handle the specific LIST 18 18 command
            send(":" + serverName + " 321 " + nick + " Channel :Users Names\r\n");

            auto channels = channellist.getChannelList();
            for (const auto& channel : channels) {
                if (channel.name.find("'s_game") != std::string::npos) {
                    size_t userCount = userlist.countUsersInChannel(channel.name);
                    std::string topic = channel.topic.empty() ? "No topic" : channel.topic;
                    send(":" + serverName + " 326 " + nick + " " + channel.name + " 1 18 18 1 0 128 :" + topic + "\r\n");


                    // Call handleNamesCommand to list users in the channel
                    handleNamesCommand({ "NAMES", channel.name });
                }
            }

            send(":" + serverName + " 323 " + nick + " :End of /LIST command\r\n");
        }
        else {
            // Handle the regular LIST command
            send(":" + serverName + " 321 " + nick + " Channel :Users Names\r\n");

            auto channels = channellist.getChannelList();
            for (const auto& channel : channels) {
                size_t userCount = userlist.countUsersInChannel(channel.name);
                std::string topic = channel.topic.empty() ? "No topic" : channel.topic;
                send(":" + serverName + " 327 " + nick + " " + channel.name + " " + std::to_string(userCount) + " 1 388 :" + topic + "\r\n");

                // Call handleNamesCommand to list users in the channel
                handleNamesCommand({ "NAMES", channel.name });
            }

            send(":" + serverName + " 323 " + nick + " :End of /LIST command\r\n");
        }
    }

    void handleAdvertrCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) return;
        send(":" + serverName + " ADVERTR 5 " + tokens[1]);
    }



    void handleModeCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 3) return;

        std::string channel = tokens[1];
        std::string mode = tokens[2];
        std::string target = tokens.size() > 3 ? tokens[3] : "";

        // Update the mode for the channel
        channelModes[channel] = mode;

        // Send the updated mode information back to the channel
        send_channel(channel, ":" + serverName + " MODE " + channel + " " + mode + " " + target);
    }


    bool dataCheck(const std::vector<std::string>& tokens, int index, int value) {
        try {
            int tokenValue = std::stoi(tokens[index]);
            return tokenValue == value;
        }
        catch (const std::invalid_argument& e) {
            std::cerr << "Invalid argument: " << e.what() << std::endl;
            return false;
        }
        catch (const std::out_of_range& e) {
            std::cerr << "Out of range: " << e.what() << std::endl;
            return false;
        }
    }

    void listUsersInChannel(const std::string& channelName) {
        if (!channellist.isExistChannel(channelName)) {
            send(":" + serverName + " 403 " + nick + " " + channelName + " :Channel does not exist.\r\n");
            return;
        }

        std::string userNames = userlist.getUserNamesInChannel(channelName);
        send(":" + serverName + " 353 " + nick + " * " + channelName + " :@" + userNames + "\r\n");
        send(":" + serverName + " 366 " + nick + " " + channelName + " :End of /NAMES list\r\n");
    }



    void listAllUsers() {
        for (const auto& channel : channellist.getChannelList()) {
            listUsersInChannel(channel.name);
        }
    }

    void listAllChannels() {
        std::string response = ":" + serverName + " 321 " + nick + " Channel :Users Name\r\n";
        for (const auto& channel : channellist.getChannelList()) {
            size_t userCount = userlist.countUsersInChannel(channel.name);
            response += ":" + serverName + " 322 " + nick + " " + channel.name + " " + std::to_string(userCount) + " :\r\n";
        }
        response += ":" + serverName + " 323 " + nick + " :End of /LIST\r\n";
        send(response);
    }
    bool valid_charset(const std::string& name) {
        for (char c : name) {
            if (!isalnum(c) && c != '_' && c != '-' && c != '*' && c != '&' && c != '#') {
                return false;
            }
        }
        return true;
    }


    void handleJoinCommand(const std::vector<std::string>& tokens) {
        try {
            if (tokens.size() < 2) {
                send(":" + serverName + " 461 " + nick + " " + tokens[0] + " :Not enough parameters");
                return;
            }
            const std::string& channelName = tokens[1];

            if (bannedUsers.find(nick) != bannedUsers.end()) {
                        send(":" + serverName + " 474 " + nick + " " + channelName + " :Cannot join channel (you're banned)");
                        send(":" + serverName + " 471 " + nick + " " + channelName + " :Cannot join channel (you're banned)");
                        return;
						std::cout << nick << " is a banned user" << channelName << std::endl;


            }
            else {
                std::cerr << "Error opening banned_users.txt for reading" << std::endl;
            }

            if (isUserBannedFromChannel(nick, channelName)) {
                send(":" + serverName + " 474 " + nick + " " + channelName + " :Cannot join channel (you're banned)");
                return;
            }

            Channel* roomToEnter = nullptr;
            bool isNewRoom = false;

            {
                std::lock_guard<std::mutex> lock(channellistMutex);
                if (channellist.isExistChannel(channelName)) {
                    roomToEnter = channellist.getChannel(channelName);
                }
                else {
                    channellist.addChannel(userlist.getUser(nick), channelName, "");
                    roomToEnter = channellist.getChannel(channelName);
                    isNewRoom = true;

                    // Add the creator as an operator
                    channelOperators[channelName].insert(nick);
                }
            }

            if (roomToEnter == nullptr) {
                send(":" + serverName + " 403 " + nick + " " + channelName + " :No such channel");
                return;
            }

            {
                std::lock_guard<std::mutex> lock(userlistMutex);
                if (!channel.empty() && channel != channelName) {
                    userlist.removeUserFromChannel(nick, channel);
                }
                userlist.addUserToChannel(nick, channelName);
                channel = channelName;
            }



            std::string joinMessage = ":" + nick + "!Username@" + serverIP + " JOIN :" + channelName + "\r\n";
            send_channel(channelName, joinMessage);
            handleNamesCommand({ "NAMES", channelName });

            if (isNewRoom) {
                send(":" + serverName + " 377 " + nick + " :Creating new room '" + channelName + "'\r\n");
                send(":" + serverName + " MODE " + channelName + " +o " + nick + "\r\n");
            }
            else {
                send(":" + serverName + " 377 " + nick + " :Joining room '" + channelName + "'\r\n");
                if (isUserOperator(channelName, nick)) {
                    send(":" + serverName + " MODE " + channelName + " +v " + nick + "\r\n");
                }
                else {
                    send(":" + serverName + " MODE " + channelName + " +v " + nick + "\r\n");
                }
            }
        }
        catch (const std::exception& ex) {
            std::cerr << "Exception in handleJoinCommand: " << ex.what() << std::endl;
            send(":" + serverName + " 400 " + nick + " :Internal server error\r\n");
        }
    }


    void handlePartCommand(const std::vector<std::string>& tokens) {
        std::cout << "Entering handlePartCommand for user: " << nick << " on channel: " << channel << std::endl;
        if (tokens.size() < 2) {
            send(":" + serverName + " 461 " + nick + " " + tokens[0] + " :Not enough parameters");
            return;
        }
        std::string channelName = tokens[1];
        try {
            if (channelName.empty()) {
                std::cerr << "Error: Channel name is empty." << std::endl;
                throw std::invalid_argument("Channel name cannot be empty");
            }
            if (!userlist.isUserInChannel(nick, channelName)) {
                send(":" + serverName + " 442 " + nick + " " + channelName + " :You're not on that channel");
                return;
            }

            std::string partMessage = ":" + nick + "!Username@" + serverIP + " PART " + channelName + "\r\n";
            send_channel(channelName, partMessage); // Broadcast part message

            userlist.removeUserFromChannel(nick, channelName);
            if (channel == channelName) {
                channel.clear();
            }

            // Send NAMES command to update all users in the channel
         //  handleNamesCommand({ "NAMES", channelName });

        }
        catch (const std::exception& e) {
            std::cerr << "Exception in handlePartCommand: " << e.what() << std::endl;
        }
        std::cout << "Exiting handlePartCommand" << std::endl;
    }
    void handleJoinGameCommand(const std::vector<std::string>& tokens) {
        try {
            // Ensure there are enough tokens for the command
            if (tokens.size() < 9) {
                send(":" + serverName + " 461 " + nick + " " + tokens[0] + " :Not enough parameters");
                return;
            }
            std::string channelName = tokens[1];
            std::ifstream banFile("banned_users.txt");
            if (banFile.is_open()) {
                std::string bannednick;
                while (std::getline(banFile, bannednick)) {
                    if (bannednick == nick + " " + channelName) {
                        send(":" + serverName + " 474 " + nick + " " + channelName + " :Cannot join channel (you're banned)");
                        send(":" + serverName + " 471 " + nick + " " + channelName + " :Cannot join channel (you're banned)");
                        return;
                    }
                }
                banFile.close();
            }
            else {
                std::cerr << "Error opening banned_users.txt for reading" << std::endl;
            }

            if (isUserBannedFromChannel(nick, channelName)) {
                send(":" + serverName + " 474 " + nick + " " + channelName + " :Cannot join channel (you're banned)");
                return;
            }
            // Extract and validate the necessary parameters
            std::string newChannelName = tokens[1];
            int minPlayers, maxPlayers, channelType;
            try {
                minPlayers = std::stoi(tokens[2]);
                maxPlayers = std::stoi(tokens[3]);
                channelType = std::stoi(tokens[4]);
            }
            catch (const std::invalid_argument&) {
                send(":" + serverName + " 461 " + nick + " " + tokens[0] + " :Invalid number format");
                return;
            }
            catch (const std::out_of_range&) {
                send(":" + serverName + " 461 " + nick + " " + tokens[0] + " :Number out of range");
                return;
            }

            std::string gameIsTournament = tokens[5];
            std::string gameExtension = (tokens.size() > 6) ? tokens[6] : "unknown";
            std::string password = (tokens.size() > 7) ? tokens[7] : "";

            Channel* roomToEnter = nullptr;
            bool isNewRoom = false;

            {
                std::lock_guard<std::mutex> lock(channellistMutex);
                if (channellist.isExistChannel(newChannelName)) {
                    roomToEnter = channellist.getChannel(newChannelName);
                }
                else {
                    channellist.addChannel(userlist.getUser(nick), newChannelName, std::to_string(maxPlayers));
                    roomToEnter = channellist.getChannel(newChannelName);
                    isNewRoom = true;
                }
            }

            if (roomToEnter == nullptr) {
                send(":" + serverName + " 403 " + nick + " " + newChannelName + " :No such channel");
                return;
            }

            size_t userCount;
            int maxPlayersInt;
            {
                std::lock_guard<std::mutex> lock(userlistMutex);
                userCount = userlist.countUsersInChannel(newChannelName);
                try {
                    maxPlayersInt = std::stoi(roomToEnter->g_maxplayers);
                }
                catch (const std::invalid_argument&) {
                    maxPlayersInt = 0; // Default to no limit if invalid
                }
                catch (const std::out_of_range&) {
                    maxPlayersInt = 0; // Default to no limit if out of range
                }

                if (userCount >= maxPlayersInt && maxPlayersInt != 0) {
                    send(":" + serverName + " 471 " + nick + " " + newChannelName + " :Channel is full.\r\n");
                    return;
                }

                if (!channel.empty() && channel != newChannelName) {
                    userlist.removeUserFromChannel(nick, channel);
                }
                userlist.addUserToChannel(nick, newChannelName);
                channel = newChannelName;
            }

            std::string joinMessage = ":" + nick + "!Username@" + serverIP + " JOINGAME :" + newChannelName + "\r\n";
            send_channel(newChannelName, joinMessage);
            handleNamesCommand({ "NAMES", newChannelName });

            if (isNewRoom) {
                //  send(":" + serverName + " 377 " + nick + " :Creating new room '" + newChannelName + "'\r\n");
                std::string joinMessage = ":" + nick + "!Username@" + serverIP + " JOINGAME :" + newChannelName + "\r\n";
                send_channel(newChannelName, joinMessage);
                handleNamesCommand({ "NAMES", newChannelName });
                send(":" + serverName + " MODE " + newChannelName + " +o " + nick + "\r\n");

            }
            else {
                std::string joinMessage = ":" + nick + "!Username@" + serverIP + " JOINGAME :" + newChannelName + "\r\n";
                send_channel(newChannelName, joinMessage);
                handleNamesCommand({ "NAMES", newChannelName });
                // send(":" + serverName + " 377 " + nick + " :Joining room '" + newChannelName + "'\r\n");
            }
            send(":" + serverName + " MODE " + newChannelName + " +v " + nick + "\r\n");

        }
        catch (const std::exception& ex) {
            std::cerr << "Exception in handleJoinGameCommand: " << ex.what() << std::endl;
            send(":" + serverName + " 400 " + nick + " :Internal server error\r\n");
        }
        catch (...) {
            std::cerr << "Unknown exception in handleJoinGameCommand" << std::endl;
            send(":" + serverName + " 400 " + nick + " :Internal server error\r\n");
        }
    }



    void handleGetCodePageCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) {
            send(":" + serverName + " 461 " + nick + " " + tokens[0] + " :Not enough parameters");
            return;
        }

        std::string response;

        for (size_t i = 1; i < tokens.size(); ++i) {
            const std::string& userNick = tokens[i];
            std::string codepage = "0";  // Default codepage value

            AnongameWolPlayer* player = getPlayer(userNick);
            if (player) {
                // Assuming codepage is represented by a specific attribute or logic for the player
                codepage = std::to_string(player->country);  // Example: using country as a placeholder
            }

            response += userNick + "`" + codepage;
            if (i < tokens.size() - 1) {
                response += "`";
            }
        }
        send(":" + serverName + " 329 " + nick + " " + response);
    }

    bool validateCommand(const std::vector<std::string>& tokens, size_t expectedParams) {
        if (tokens.size() < expectedParams) {
            send(":" + serverName + " 461 " + nick + " " + tokens[0] + " :Not enough parameters");
            return false;
        }
        return true;
    }


    bool isUserInCorrectChannel(const std::string& targetChannel) {
        if (channel != targetChannel) {
            send(":" + serverName + " 442 " + nick + " " + targetChannel + " :You're not in that channel");
            return false;
        }
        return true;
    }



    void sendCommandToChannel(const std::string& targetChannel, const std::string& command) {
        if (channellist.isExistChannel(targetChannel)) {
            send_channel(targetChannel, command);
        }
        else {
            send(":" + serverName + " 403 " + nick + " " + targetChannel + " :No such channel");
        }
    }


    void handleGameOptCommand(const std::vector<std::string>& tokens) {
        try {
            std::cout << "GAMEOPT command received. Tokens: ";
            for (const auto& token : tokens) {
                std::cout << token << " ";
            }
            std::cout << std::endl;

            if (!validateCommand(tokens, 3)) return;

            std::string target = tokens[1];
            std::string gameOptions = tokens[2];
            for (size_t i = 3; i < tokens.size(); ++i) {
                gameOptions += " " + tokens[i];
            }

            // Remove leading colon from gameOptions if present
            if (!gameOptions.empty() && gameOptions[0] == ':') {
                gameOptions = gameOptions.substr(1);
            }

            std::cout << "Target: " << target << ", GameOptions: " << gameOptions << std::endl;

            if (!isUserInCorrectChannel(target)) return;

            // Check if the command is a recognized GAMEOPT command
            if (gameOptions.find("R0,") == 0 || gameOptions.find("R1,") == 0 ||
                gameOptions.find("R0,0") == 0 || gameOptions.find("R0,1") == 0 ||
                gameOptions.find("R0,2") == 0 || gameOptions.find("R0,3") == 0 ||
                gameOptions.find("R0,4") == 0 || gameOptions.find("R0,5") == 0 ||
                gameOptions.find("R0,6") == 0 || gameOptions.find("R0,7") == 0 ||
                gameOptions.find("R0,8") == 0 || gameOptions.find("R0,9") == 0 ||
                gameOptions.find("R1,0") == 0 || gameOptions.find("R1,1") == 0 ||
                gameOptions.find("R1,2") == 0 || gameOptions.find("R1,3") == 0 ||
                gameOptions.find("R1,4") == 0 || gameOptions.find("R1,5") == 0 ||
                gameOptions.find("R1,6") == 0 || gameOptions.find("R1,7") == 0 ||
                gameOptions.find("R1,8") == 0 || gameOptions.find("R1,9") == 0 ||
                gameOptions.find("R1,5") == 0) {
                // Handle recognized GAMEOPT commands
                if (target[0] == '#') {
                    // Channel talk
                    sendCommandToChannel(target, ":" + nick + "!UserName@" + serverName + " GAMEOPT " + target + " :" + gameOptions);
                }
                else {
                    // Whisper
                    if (userlist.isUserExist(target)) {
                        User recipient = userlist.getUser(target);
                        std::string message = ":" + nick + "!UserName@" + serverName + " GAMEOPT " + nick + " :" + gameOptions;
                        send_to_user(recipient, message);
                    }
                    else {
                        send(":" + serverName + " 401 " + nick + " " + target + " :No such nick/channel");
                    }
                }
            }
            else {
                // Unrecognized GAMEOPT command
                send(":" + serverName + " 421 " + nick + " " + tokens[0] + " :Unknown command");
            }
        }
        catch (const std::exception& ex) {
            std::cerr << "Exception in handleGameOptCommand: " << ex.what() << std::endl;
            send(":" + serverName + " 400 " + nick + " :Internal server error\r\n");
        }
        catch (...) {
            std::cerr << "Unknown exception in handleGameOptCommand" << std::endl;
            send(":" + serverName + " 400 " + nick + " :Internal server error\r\n");
        }
    }





    void handlePageCommand(const std::vector<std::string>& tokens) {

        if (tokens.size() < 3) {
            std::cout << "PAGE command received with insufficient parameters." << std::endl;
            send(":" + serverName + " 461 " + tokens[0] + " PAGE :Not enough parameters");
            return;
        }

        std::string targetNick = tokens[1];
        std::string messageText = tokens[2];
        bool paged = false;

        /*  if (targetNick == "0") {
              // PAGE for MY BATTLECLAN
              Clan* clan = Clan::getClan(0); // Replace 0 with actual account retrieval logic
              if (clan && clan->sendMessageToOnlineMembers(0, nullptr, messageText) >= 1) {
                  paged = true;
              }
          }
          else {*/
          // Check if the target user exists
        if (userlist.isUserExist(targetNick)) {
            User targetUser = userlist.getUser(targetNick);

            // Check if the user allows paging
            if (targetUser.allowsPage) {
                // Send the PAGE message to the target user
                send(":" + targetUser.nick + "!UserName@" + serverName + " PAGE " + targetNick + " :" + messageText);
                paged = true;
                //  }
            }
        }

        if (paged) {
            send(":" + serverName + " 389 " + tokens[0] + " 0");
        }
        else {
            send(":" + serverName + " 389 " + tokens[0] + " 1");
        }
    }

    void logMessage(const std::string& sender, const std::string& recipient, const std::string& message) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::ofstream logFile("private_messages.log", std::ios_base::app);
        if (logFile.is_open()) {
            logFile << "[" << getCurrentDateTime() << "] " << sender << " to " << recipient << ": " << message << std::endl;
            logFile.close();
        }
        else {
            std::cerr << "Error opening log file for writing." << std::endl;
        }
    }

    void handlePrivMsgCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 4) {
            send("Not enough parameters for PRIVMSG command");
            return;
        }

        const std::string& target2 = tokens[1];
        const std::string& message2 = tokens[3]; // tokens[3] contains the actual message after ":"

        if (target2 == "matchbot" || target2 == nick) {
            handleMatchbotCommand(message2);
        }
        else {

        std::string target = tokens[1];
        std::string message = tokens[2];
        for (size_t i = 3; i < tokens.size(); ++i) {
            message += " " + tokens[i];
        }

        if (message[0] == ':') {
            message = message.substr(1);
        }

        std::cout << "Sending PRIVMSG from " << nick << " to " << target << ": " << message << std::endl;

        if (target[0] == '#') {
            // Message to a channel
            if (channellist.isExistChannel(target)) {
                std::string fullMessage = ":" + nick + "!Username@" + serverIP + " PRIVMSG " + target + " :" + message;
                send_channel(target, fullMessage); // Broadcast message to all users in the channel
            }
            else {
                std::cout << "Channel " << target << " does not exist." << std::endl;
                send(":" + serverName + " 403 " + nick + " " + target + " :No such channel");
            }
        }
        else {
            // Message to a specific user
            if (userlist.isUserExist(target)) {
                User recipient = userlist.getUser(target);
                std::string fullMessage = ":" + nick + "!Username@" + serverIP + " PRIVMSG " + target + " :" + message;
                send_to_user(recipient, fullMessage);
                logMessage(nick, target, message);
                send(":" + serverName + " NOTICE " + nick + " :Message delivered to " + target);
            }
            else {
                std::cout << "User " << target << " does not exist." << std::endl;
                send(":" + serverName + " 401 " + nick + " " + target + " :No such nick/channel");
            }
        }
        }
    }



    void handleFindUserExCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) return;
        std::string user = tokens[1];
        send(":" + serverName + " 398 " + nick + " 0 :" + userlist.getChannelName(user));
    }

    void handleFindUserCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) return;
        std::string user = tokens[1];
        if (!userlist.getChannelName(user).empty()) {
            send(":" + serverName + " 388 " + nick + " " + user + " :" + userlist.getChannelName(user));
        }
        else {
            send(":" + serverName + " 401 " + nick + " " + user + " :No such nick/channel");
        }
    }

    void handleTopicCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 3) return;
        std::string channel = tokens[1];
        std::string topic = tokens[2];
        send_channel(channel, ":" + nick + "!Username@" + serverIP + " TOPIC " + channel + " :" + topic);
    }


    void handleStartGCommand(const std::vector<std::string>& tokens) {
        try {
            if (tokens.size() < 2) {
                send(":" + serverName + " 461 " + nick + " " + tokens[0] + " :Not enough parameters");
                return;
            }

            std::string channelName = tokens[1];
            std::string ownerNick = tokens.size() > 2 ? tokens[2] : nick;
            Channel* channel = channellist.findChannel(channelName);

            if (!channel) {
                send(":" + serverName + " 403 " + nick + " " + channelName + " :No such channel");
                return;
            }

            std::vector<User> usersInChannel = userlist.getUsersInChannel(channelName);
            if (usersInChannel.empty()) {
                send(":" + serverName + " 366 " + nick + " " + channelName + " :No users in channel.");
                return;
            }

            time_t startTime = std::time(nullptr);
            std::string gameNumber = "1"; // Placeholder for game number

            if (clientVersion == 0) {
                // WOLv1: :user1!Username@hostname STARTG u :owner_ip gameNumber time_t
                std::string response = ":" + ownerNick + "!Username@" + serverName + " STARTG u :" + userlist.getUser(ownerNick).ip + " " + gameNumber + " " + std::to_string(startTime) + "\r\n";
                send(response);
            }
            else {
                // WOLv2: :user1!Username@hostname STARTG u :user1 xxx.xxx.xxx.xxx user2 xxx.xxx.xxx.xxx :gameNumber time_t
                std::string response = ":" + ownerNick + "!Username@" + serverName + " STARTG u :" + ownerNick + " " + userlist.getUser(ownerNick).ip;
                for (const auto& user : usersInChannel) {
                    if (user.nick != ownerNick) {
                        response += " " + user.nick + " " + user.ip;
                    }
                }
                response += " :" + gameNumber + " " + std::to_string(startTime) + "\r\n";
                send(response);
            }
        }
        catch (const std::exception& ex) {
            std::cerr << "Exception in handleStartGCommand: " << ex.what() << std::endl;
            send(":" + serverName + " 400 " + nick + " :Internal server error\r\n");
        }
        catch (...) {
            std::cerr << "Unknown exception in handleStartGCommand" << std::endl;
            send(":" + serverName + " 400 " + nick + " :Internal server error\r\n");
        }
    }



    void handlecversCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 1) return;
        send(":" + serverName + " 421 " + nick + " " + tokens[0] + " :Unknown Command");
        send(":" + serverName + " -> Check Passed.");
        //  Sleep(500);

    }

    void handleChanChkCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) return;
        std::string channel = tokens[1];
        if (channellist.isExistChannel(channel)) {
            send(":" + serverName + " 331 " + nick + " " + channel + " :No topic is set");
        }
        else {
            send(":" + serverName + " 403 " + nick + " " + channel + " :No such channel");
        }
    }
    void handleSquadInfoCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) {
            send(":" + serverName + " 461 " + nick + " " + tokens[0] + " :Not enough parameters");
            return;
        }

        int clanId;
        if (tokens[1] == "0") {
            clanId = getUserClanId(nick); // Assume this function gets the user's clan ID
        }
        else {
            try {
                clanId = std::stoi(tokens[1]);
            }
            catch (const std::out_of_range&) {
                std::cerr << "Out of range error" << std::endl;
                return;
            }

        }

        ClanDetails clanDetails = getClanInfo(clanId); // Assume this function retrieves clan details
        if (clanDetails.ID > 0) {
            std::string tempStr = std::to_string(clanDetails.ID) + "`" + clanDetails.Name + "`" + clanDetails.Tag + "`" +
                std::to_string(clanDetails.Rank) + "`0`1`0`0`0";
            send(":" + serverName + " 358 " + nick + " " + tempStr + "`0`0`0`0`x`x`x");
        }
        else {
            send(":" + serverName + " 439 " + nick + " :ID doesn't exist.");
        }
    }

    void handleGetBuddyCommand(const std::vector<std::string>& tokens) {
        send(":" + serverName + " 333 " + nick + " ");
    }

    void handleAddBuddyCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) {
            send(":" + serverName + " 461 " + nick + " " + tokens[0] + " :Not enough parameters");
            return;
        }
        send(":" + serverName + " 334 " + nick + " " + tokens[1]);
    }

    void handleDelBuddyCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) {
            send(":" + serverName + " 461 " + nick + " " + tokens[0] + " :Not enough parameters");
            return;
        }
        send(":" + serverName + " 335 " + nick + " " + tokens[1]);
    }


    void handleQuitCommand(const std::vector<std::string>& tokens) {
        std::string quitMessage = ":" + nick + "!Username@" + serverIP + " QUIT :Goodbye!";
        send(quitMessage);

        // Remove the user from the user list and all channels
        std::string userChannel = userlist.getChannelName(nick);
        if (!userChannel.empty()) {
            std::string partMessage = ":" + nick + "!Username@" + serverIP + " PART :" + userChannel + "\r\n";
            send_channel(userChannel, partMessage); // Broadcast part message to all users in the channel
            userlist.removeUserFromChannel(nick, userChannel);
        }
        userlist.removeUser(nick);  // Ensure the user is removed from the userlist

        closesocket(socket);
        endThread = true;
    }

    void handlePingCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) return;
        send(":" + serverName + " PONG " + serverName + " :" + tokens[1]);
    }

    void send(const std::string& message) {
        std::string msg = message + "\r\n";
        ::send(socket, msg.c_str(), static_cast<int>(msg.length()), 0);
        AppendTextToEditControl(hwndEdit, msg); // Append to log window
    }

    void send_channel(const std::string& channel, const std::string& message) {
        auto users = userlist.getUsersInChannel(channel);
        for (const auto& user : users) {
            send_to_user(user, message);
        }
    }



    void send_channel_others(const std::string& channel, const std::string& message) {
        auto users = userlist.getUsersInChannel(channel);
        for (const auto& user : users) {
            if (user.nick != nick) { // Ensure the message is not sent to the sender
                send_to_user(user, message);
            }
        }
    }

    void send_to_user(const User& user, const std::string& message) {
        std::string msg = message + "\r\n";
        ::send(socket, msg.c_str(), static_cast<int>(msg.length()), 0); // Ensure correct socket is used
    }

};

void RunServerLoop(SOCKET listenSocket, UserList& userlist, ChannelList& channellist) {
    while (serverRunning) {
        SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "accept failed with error: " << WSAGetLastError() << std::endl;
            continue;
        }
        ServerUserClient* client = new ServerUserClient(clientSocket, userlist, channellist, "AWOS", "127.0.0.1", "4010", "1.0");
        std::thread clientThread(&ServerUserClient::run, client);
        clientThread.detach();
    }
}

void StartServer() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed with error: " << result << std::endl;
        return;
    }

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "socket failed with error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(4010);

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "bind failed with error: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "listen failed with error: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    UserList userlist;
    ChannelList channellist;

    serverThread = std::thread(RunServerLoop, listenSocket, std::ref(userlist), std::ref(channellist));
}

void StopServer() {
    serverRunning = false;
    closesocket(listenSocket);
    if (serverThread.joinable()) {
        serverThread.join();
    }
    WSACleanup();
}

void RestartServer() {
    StopServer();
    StartServer();
}


int main() {
    SessionManager sessionManager;
    readBannedUsers();
    std::cout << "AWOS initiated: Main WOLV2 Servers online" << std::endl;
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed with error: " << result << std::endl;
        return 1; // Return a non-zero value to indicate an error
    }

    // Start a thread to clean up inactive sessions
    std::thread cleanupThread(&SessionManager::cleanupInactiveSessions, &sessionManager, std::chrono::minutes(10));

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(4010);

    bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(listenSocket, SOMAXCONN);

    UserList userlist;
    ChannelList channellist;
    ThreadPool threadPool(8); // Create a thread pool with 8 threads
    std::ifstream banFile("banned_users.txt");
    if (banFile.is_open()) {
        std::string bannedNick;
        while (std::getline(banFile, bannedNick)) {
            bannedUsers.insert(bannedNick);
        }
        banFile.close();
    }
    else {
        std::cerr << "Error opening banned_users.txt for reading" << std::endl;
    }

    for (int i = 0; i < 10; ++i) {
        threadPool.enqueue(incrementCounter);
    }

    while (true) {
        SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
        ++SocketTotal; // Increment connection count
        threadPool.enqueue([clientSocket, &userlist, &channellist, &sessionManager]() {
            ServerUserClient client(clientSocket, userlist, channellist, "AWOS", "127.0.0.1", "4010", "1.0");
            sessionManager.addSession(std::to_string(clientSocket), client.getNick()); // Add session
            client.run();
            sessionManager.removeSession(std::to_string(clientSocket)); // Remove session on disconnect
            });
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Stop the session manager and join the cleanup thread
    sessionManager.stop();
    cleanupThread.join();
   


    WSACleanup();
    return 0; // Return 0 to indicate successful execution
}
