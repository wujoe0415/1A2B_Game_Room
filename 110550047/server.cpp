#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

#define MAX_CLIENT 20
using namespace std;

struct playerInfo{
    string name = "";
    string email = "";
    bool isInGame = false;
    string inRoomId = "";
    bool isOnline = false;
    map<string, int> invitation; // <roomID, clientID>
    void Init(string _name, string _email){
        name = _name;
        email = _email;
        isInGame = false;
        inRoomId = "";
        isOnline = true;
        //invitation.clear();
    }
    void Clear(){
        name = "";
        email = "";
        isInGame = false;
        inRoomId = "";
        isOnline = false;
        //invitation.clear();
    }
};
class Game{
public:
    Game(){
    }
    void StartGame(vector<int> player, string round, string specificNumber = ""){
        if(specificNumber == ""){
            int random = (rand() % 9000) + 1000;
            while(!isValidNumber(random)){
                random = (rand() % 9000) + 1000;
            }
            targetNumber = to_string(random);
        }
        else
            targetNumber = specificNumber;
        
        guessOrder = 0;
        currentRound = 1;
        isEnded = false;
        maxRound = stoi(round);
        players.clear();
        players = player;
    }
    string Guess(string number){
        int Anumber = 0, Bnumber = 0;
        bool visited[4] = {0};
        for(int i = 0;i < 4;i++)
            visited[i] = false;
        for(int i = 0;i<4;i++){
            if(targetNumber[i] == number[i]){
                visited[i] = true;
                Anumber++;
            }
        }
        if(Anumber == 4){
            isEnded = true;
            return "Bingo";
        }
        for(int i = 0 ; i < 4 ; i++){
            if(visited[i])
                continue;
            for(int j = 0 ; j < 4 ; j++){
                if(i == j)
                    continue;
                if(!visited[j] && number[i] == targetNumber[j]){
                    Bnumber++;
                    break;
                }
            }
        }
        guessOrder++;

        if(guessOrder == players.size()){
            guessOrder = 0;
            currentRound++;
            if(currentRound == maxRound + 1) // game end
                isEnded = true;
        }
        return to_string(Anumber) + "A" + to_string(Bnumber) + "B";
    }
    void QuitGame(){
    }
    int getCurrentPlayer(){
        return players[guessOrder];
    }
    //bool isEnded = false;
    vector<int> players;
    bool isEnded = false;
private:
    int maxRound = 0;
    int currentRound = 0;
    int guessOrder = 0;
    string targetNumber;
    
    bool isValidNumber(int number){
        int a, b, c, d;
        a = (number%10);
        number /= 10;
        b = (number%10);
        number /= 10;
        c = (number%10);
        number /= 10;
        d = (number%10);
        if(a==b || b==c || c==d || d==a)
            return false;

        return true;
    }
};
class GameRoom{
public:
    bool isPublic = false;
    bool isInGame = false;
    string GameRoomId;
    string invitationCode;
    int GameManager = -1;
    vector<int> players;
    GameRoom(){}
    GameRoom(bool ispublic, string id, int manager, string code= ""){
        isPublic = ispublic;
        isInGame = false;
        GameRoomId = id;
        GameManager = manager;
        invitationCode = code;
        players.clear();
        game = new Game();
    }
    ~GameRoom(){
        QuitGame();
        isPublic = false;
        isInGame = false;
        GameRoomId = "";
        GameManager = -1;
        invitationCode = "";
        players.clear();
        //game = new Game();
    }
    void StartGame(string round, string targetNumber = ""){
        isInGame = true;
        game->StartGame(players, round, targetNumber);
    }
    void QuitGame(){
        isInGame = false;
        delete(game);
        game = new Game();
    }
    Game* game;
private:
};
class UDPProtocol{
public:
    UDPProtocol(){
        udpSockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if(udpSockfd == -1)
        {
            perror("Server failed to create UDPsocket");
            exit(EXIT_FAILURE);
        }
    }
    void Bind(){
        if(bind(udpSockfd, (const struct sockaddr*)serverAddress, sizeof(*serverAddress)) < 0){
            perror("Server fail to bind UDP");
            Close();
            exit(EXIT_FAILURE);
        }
    }
    void SetServerAddress(struct sockaddr_in* sa){
        serverAddress = sa;
    }
    void SendMessage(string mes){
        sendto(udpSockfd, mes.c_str(), mes.size(), 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));
    }
    string ReceiveMessage(){
        len = sizeof(clientAddress);
        int n = recvfrom(udpSockfd, (char*)buffer, 50, 0, (struct sockaddr*)&clientAddress, (socklen_t*)&len);
        buffer[n] = '\0';
        //cout<<"udp Receive message"<<endl;
        //cout<<buffer<<endl;
        return buffer;
    }
    void Close(){
        close(udpSockfd);
    }
    int udpSockfd = 0;
private:
    struct sockaddr_in* serverAddress={0};
    struct sockaddr_in clientAddress = {0};
    char buffer[1024]={0};
    int len = 0;
    //socklen_t len = 0;
};
class TCPProtocol{
public: 
    TCPProtocol(){
        tcpSockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(tcpSockfd == -1)
        {
            perror("Server failed to create UDPsocket");
            exit(EXIT_FAILURE);
        }
        for(int i = 0; i<MAX_CLIENT + 10;i++)
            configSockfds[i] = -1;
    }
    void SetServerAddress(struct sockaddr_in* sa){
        serverAddress = sa;
    }
    void Bind(){
        if (bind(tcpSockfd, (struct sockaddr*)serverAddress, sizeof(*serverAddress)) < 0) {
            printf("TCP socket bind failed...\n");
            exit(0);
        }
    }
    void Listen(){
        if (listen(tcpSockfd, MAX_CLIENT) < 0) {
            printf("Listen failed...\n");
            exit(0);
        }
    }
    int Accept(){
        int len = sizeof(clientAddress);
        int newSocket = accept(tcpSockfd, (struct sockaddr*) &clientAddress, (socklen_t*)&len);
        
        if (newSocket < 0) {
            printf("TCP server accept failed...\n");
            exit(0);
        }
        else{
            for(int i = 0;i< MAX_CLIENT + 10;i++){
                if(configSockfds[i] == -1){
                    configSockfds[i] = newSocket;
                    break;
                }
            }       
        }
        return newSocket;
    }
    void Broadcast(string mes, vector<int> clients, int client){
        for(int mclient : clients)
            SendMessage(mes, mclient);
        //SendMessage("(Broadcast to " + to_string(clients.size()) + ") " + mes, client);
    }
    void SendMessage(string mes, int clientSocketfd){
        write(configSockfds[clientSocketfd], mes.c_str(), mes.size());
    }
    string ReceiveMessage(int clientSocketfd){
        int val;
        if((val = read(configSockfds[clientSocketfd], buffer, sizeof(buffer)))==0){
            //cout<<"TCP exit\n";
            // close(configSockfds[clientSocketfd]);
            // configSockfds[clientSocketfd] = -1;
            buffer[val] = 0;
            return "exit";
        }
        buffer[val] = '\0';
        return buffer;
    }
    void Close(int clientSocketfd){
        close(configSockfds[clientSocketfd]);
        configSockfds[clientSocketfd] = -1;
    }
    void Close(){
        close(tcpSockfd);
        tcpSockfd = 0;
        for(int i = 0;i<MAX_CLIENT + 10 ; i++)
            configSockfds[i] = -1;
    }
    int tcpSockfd = 0;
    // int configSockfd = 0;
    int configSockfds[MAX_CLIENT + 10] = {0};
private:
    struct sockaddr_in* serverAddress={0};
    struct sockaddr_in clientAddress;
    char buffer[1024]={0};
    socklen_t len = 0;
};

class Server{
public:
    Server(int port){
        Port = port;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddress.sin_port = htons(port);
        
        TCP();
        UDP();
        nready = 0;
    };
    void LogicLoop(){
        while(true){

            FD_ZERO(&rset); //Clear the socket set
            FD_SET(masterTCPSocket->tcpSockfd, &rset);
	    	FD_SET(uProtocol.udpSockfd, &rset);

            // set listenfd and udpfd in readset
            for(int i = 0;i < MAX_CLIENT;i++){
                if(masterTCPSocket->configSockfds[i] < 0)
                    continue;
                FD_SET(masterTCPSocket->configSockfds[i], &rset);
            }

            int maxfdp1 = max(masterTCPSocket->tcpSockfd, uProtocol.udpSockfd);
            for(int i = 0 ; i < MAX_CLIENT ; i++){
                if(masterTCPSocket->configSockfds[i] < 0)
                    continue;
                maxfdp1 = max(masterTCPSocket->configSockfds[i], maxfdp1);
            }
            nready = select(maxfdp1 + 1, &rset, NULL, NULL, NULL);
            
            if(nready < 0)
                cout << "select error.\n";

            // master TCP socket
            if (FD_ISSET(masterTCPSocket->tcpSockfd, &rset))  
            {  
                int newSocket = masterTCPSocket->Accept();
                
                // cout << "New connection.\n";
                // string welcome_mes = "*****Welcome to Game 1A2B*****";
                // // //send new connection greeting message 
                // if( send(newSocket, welcome_mes.c_str(), welcome_mes.size(), 0) != welcome_mes.size())  
                // {  
                //     perror("Failed to send welcome meassage\n");  
                // }  
                // Add new socket to array of sockets 
                //AddTCPClient(newSocket); 
            }
            // UDP
            if (FD_ISSET(uProtocol.udpSockfd, &rset)) {
                string rcvmes = uProtocol.ReceiveMessage();
                HandleCommand(rcvmes);
            }
            // TCP
            for(int i = 0; i< MAX_CLIENT;i++){
                if(masterTCPSocket->configSockfds[i] == -1)
                    continue;
                if (FD_ISSET(masterTCPSocket->configSockfds[i], &rset)) {
                    string rcvmes = masterTCPSocket->ReceiveMessage(i);
                    HandleCommand(rcvmes, i);
                }
            }
        }
    };
    void HandleCommand(string command, int tcpNum = -1){
        vector<string> cmds = SplitCommand(command);
        if(tcpNum == -1){
            if(cmds[0] == "register")
                Register(cmds);
            else if(cmds[0] == "game-rule")
                GameRule();
            else if(cmds[0] == "list" && cmds[1] == "rooms")
                ListRooms();
            else if(cmds[0] == "list" && cmds[1] == "users")
                ListUsers();
            else
                uProtocol.SendMessage("Not a valid command : " + command + "\n");
        }
        else /*if(!players[tcpNum].isInGame)*/{
            if(cmds[0] == "login")
                Login(cmds, tcpNum);
            else if(cmds[0] == "logout")
                Logout(tcpNum);
            else if(cmds[0] == "create" && cmds[1] == "public" && cmds[2] == "room")
                CreatePublicRoom(cmds, tcpNum);
            else if(cmds[0] == "create" && cmds[1] == "private" && cmds[2] == "room")
                CreatePrivateRoom(cmds, tcpNum);
            else if(cmds[0] == "join" && cmds[1] == "room")
                JoinRoom(cmds, tcpNum);
            else if(cmds[0] == "invite")
                Invite(cmds, tcpNum);
            else if(cmds[0] == "list" && cmds[1] == "invitations")
                ListInvitations(tcpNum);
            else if(cmds[0] == "accept")
                AcceptInvitation(cmds, tcpNum);
            else if(cmds[0] == "leave" && cmds[1] == "room")
                LeaveRoom(tcpNum);
            else if(cmds[0] == "start" && cmds[1] == "game")
                StartGame(cmds, tcpNum);
            else if(cmds[0] == "exit")
                Exit(tcpNum);
            else if(cmds[0] == "guess")
                Guess(cmds, tcpNum);
            else
                masterTCPSocket->SendMessage("Not a valid command : " + command + "\n", tcpNum);
        }
        // In Game
        // else{
        //     string num = cmds[0];
        //     if(!isValidGuess(num))
        //         masterTCPSocket->SendMessage("Your guess should be a 4-digit number.", tcpNum);
        //     else{
        //         masterTCPSocket->SendMessage(games[tcpNum].Guess(num), tcpNum);
        //         if(games[tcpNum].isEnded)
        //             players[tcpNum].isInGame = false;
        //     }
        //}
    }
    int Port;

private:
    void Register(vector<string> cmds){
        if(cmds.size() != 4) {
			uProtocol.SendMessage("Usage: register <username> <email> <password>\n");
			return;
		}
		if(user2data.find(cmds[1]) != user2data.end()) {
			uProtocol.SendMessage("Username or Email is already used\n");
			return;
		}
		if(email2data.find(cmds[2]) != email2data.end()) {
			uProtocol.SendMessage("Username or Email is already used\n");
			return;
		}
		email2data[cmds[2]] = {cmds[1], cmds[3]};
		user2data[cmds[1]] = {cmds[2], cmds[3]};

	    uProtocol.SendMessage("Register Successfully\n");
    }
    void GameRule(){
        uProtocol.SendMessage("1. Each question is a 4-digit secret number.\n2. After each guess, you will get a hint with the following information:\n2.1 The number of \"A\", which are digits in the guess that are in the correct position.\n2.2 The number of \"B\", which are digits in the guess that are in the answer but are in the wrong position.\nThe hint will be formatted as \"xAyB\".\n3. 5 chances for each question.");
    }
    void ListRooms(){
        string list = "List Game Rooms\n";
        if(rooms.size() == 0)
        {
            uProtocol.SendMessage(list + "No Rooms\n");
            return;
        }
        string roomList = "";
        int count = 0;
        for(auto it = rooms.begin(); it != rooms.end(); it++){
            if(it->second->GameRoomId != ""){
                auto room = it->second;
                count++;
                if(room->isPublic && room->isInGame)
                    roomList += to_string(count) + ". (Public) Game Room " + it->first+ " has started playing\n";
                else if(room->isPublic && !room->isInGame)
                    roomList += to_string(count) + ". (Public) Game Room " + it->first+ " is open for players\n";
                else if(!room->isPublic && room->isInGame)
                    roomList += to_string(count) + ". (Private) Game Room " + it->first+ " has started playing\n";
                else    
                    roomList += to_string(count) + ". (Private) Game Room " + it->first+ " is open for players\n";
            }
        }
        uProtocol.SendMessage(list + roomList);
        
    }
    void ListUsers(){
        string list = "List Users\n";
        string userList = "";
        int count = 0;
        vector<string> onlineUsers;

        for(auto player :players)
        {
            if(player.name != "")
                onlineUsers.push_back(player.name);
        }
        for(auto it = user2data.begin(); it != user2data.end() ; it++)
        {
            count++;
            if(find(onlineUsers.begin(), onlineUsers.end(), it->first) != onlineUsers.end())
                userList += to_string(count) + ". " + it->first + "<" + it->second.first + "> Online\n";
            else
                userList += to_string(count) + ". " + it->first + "<" + it->second.first + "> Offline\n";
        }
        if(userList != ""){
            uProtocol.SendMessage( list + userList);
        }
        else
            uProtocol.SendMessage(list +"No Users\n");
    }
    void Login(vector<string> cmds, int clientIndex){
        if(cmds.size() != 3) {
			masterTCPSocket->SendMessage("Usage: login <username> <password>\n", clientIndex);
			return;
		}
		if(user2data.find(cmds[1]) == user2data.end()) {
			masterTCPSocket->SendMessage("Username does not exist\n", clientIndex);
			return;
		}
		if(user2data[cmds[1]].second != cmds[2]) {
			masterTCPSocket->SendMessage("Wrong password\n", clientIndex);
			return;
		}
        if(players[clientIndex].name != "") {
			masterTCPSocket->SendMessage("You already logged in as " + players[clientIndex].name + "\n", clientIndex);
			return;
		}
        for(int i = 0;i < MAX_CLIENT ; i++){
            if(players[i].name == cmds[1])
            {
                masterTCPSocket->SendMessage("Someone already logged in as " + cmds[1] + "\n", clientIndex);
                return;
            }
        }
        masterTCPSocket->SendMessage("Welcome, " + cmds[1] + "\n", clientIndex);
		players[clientIndex].Init(cmds[1], user2data[cmds[1]].first);
    }
    void Logout(int clientIndex){
        if(players[clientIndex].name == "") {
			masterTCPSocket->SendMessage("You are not logged in\n", clientIndex);
            return;
		}
        if(players[clientIndex].inRoomId != "")
            masterTCPSocket->SendMessage("You are already in game room "+ players[clientIndex].inRoomId + ", please leave game room\n", clientIndex);
		else {
			masterTCPSocket->SendMessage("Goodbye, " + players[clientIndex].name + "\n", clientIndex);
			players[clientIndex].Clear();
		}
    }
    void CreatePublicRoom(vector<string> cmds, int clientIndex){
        if(players[clientIndex].name == ""){
            masterTCPSocket->SendMessage("You are not logged in\n", clientIndex);
            return;
        }
        if(players[clientIndex].inRoomId != ""){
            masterTCPSocket->SendMessage("You are already in game room " + players[clientIndex].inRoomId + ", please leave game room\n", clientIndex);
            return;
        }
        if(rooms.find(cmds[3]) != rooms.end())
        {
            masterTCPSocket->SendMessage("Game room ID is used, choose another one\n", clientIndex);
            return;
        }
        rooms[cmds[3]] = new GameRoom(true, cmds[3], clientIndex);
        rooms[cmds[3]]->players.push_back(clientIndex);
        players[clientIndex].inRoomId = cmds[3];
        masterTCPSocket->SendMessage("You create public game room " + cmds[3] + "\n", clientIndex);

    }
    void CreatePrivateRoom(vector<string> cmds, int clientIndex){
        if(cmds.size() != 5)
        {
            masterTCPSocket->SendMessage("Wrong Input\n", clientIndex);
            return;
        }
        if(players[clientIndex].name == ""){
            masterTCPSocket->SendMessage("You are not logged in\n", clientIndex);
            return;
        }
        if(players[clientIndex].inRoomId != ""){
            masterTCPSocket->SendMessage("You are already in game room " + players[clientIndex].inRoomId + ", please leave game room\n", clientIndex);
            return;
        }
        if(rooms.find(cmds[3]) != rooms.end())
        {
            masterTCPSocket->SendMessage("Game room ID is used, choose another one\n", clientIndex);
            return;
        }
        rooms[cmds[3]] = new GameRoom(false, cmds[3], clientIndex, cmds[4]);
        rooms[cmds[3]]->players.push_back(clientIndex);
        players[clientIndex].inRoomId = cmds[3];
        masterTCPSocket->SendMessage("You create private game room " + cmds[3] + "\n", clientIndex);
    }
    void JoinRoom(vector<string> cmds, int clientIndex){
        if(players[clientIndex].name == ""){
            masterTCPSocket->SendMessage("You are not logged in\n", clientIndex);
            return;
        }
        if(players[clientIndex].inRoomId != ""){
            masterTCPSocket->SendMessage("You are already in game room " +  players[clientIndex].inRoomId + ", please leave game room\n", clientIndex);
            return;
        }
        if(rooms.find(cmds[2]) == rooms.end()){
            masterTCPSocket->SendMessage("Game room " + cmds[2] + " is not exist\n", clientIndex);
            return;
        }
        if(!rooms.find(cmds[2])->second->isPublic)
        {
            masterTCPSocket->SendMessage("Game room is private, please join game by invitation code\n", clientIndex);
            return;
        }
        if(rooms.find(cmds[2])->second->isInGame)
        {
            masterTCPSocket->SendMessage("Game has started, you can't join now\n",clientIndex);
            return;
        }

        for(const auto player : rooms[cmds[2]]->players)
            masterTCPSocket->SendMessage("Welcome, " + players[clientIndex].name + " to game!\n", player);
        
        players[clientIndex].inRoomId = cmds[2];
        masterTCPSocket->SendMessage("You join game room " + cmds[2] + "\n", clientIndex);
        rooms[cmds[2]]->players.push_back(clientIndex);
    }
    void Invite(vector<string> cmds, int clientIndex){
        if(players[clientIndex].name == "")
        {
            masterTCPSocket->SendMessage("You are not logged in\n", clientIndex);
            return;
        }
        if(players[clientIndex].inRoomId == ""){
            masterTCPSocket->SendMessage("You did not join any game room\n", clientIndex);
            return;
        }
        if(rooms[players[clientIndex].inRoomId]->GameManager != clientIndex && rooms[players[clientIndex].inRoomId]->isPublic)
        {
            masterTCPSocket->SendMessage("You are not private game room manager\n", clientIndex);
            return;
        }
        for(int i = 0 ; i < MAX_CLIENT ; i++){
            if(players[i].email == cmds[1])
            {
                masterTCPSocket->SendMessage("You send invitation to "+ players[i].name + "<"+ players[i].email +">\n", clientIndex);
                players[i].invitation[players[clientIndex].inRoomId] = clientIndex;
                masterTCPSocket->SendMessage("You receive invitation from "+ players[clientIndex].name + "<"+ players[clientIndex].email +">\n", i);
                return;
            }
        }
        masterTCPSocket->SendMessage("Invitee not logged in\n", clientIndex);
        return;
    }
    void ListInvitations(int clientIndex){
        if(players[clientIndex].name == "") {
			masterTCPSocket->SendMessage("You are not logged in\n", clientIndex);
			return;
		}
        string mes = "List invitations\n";
        int count = 0;
        for(auto it = players[clientIndex].invitation.begin() ; it != players[clientIndex].invitation.end() ; it++){
            if(it->first == players[it->second].inRoomId){
                count++;
                mes += to_string(count) + ". " + players[it->second].name + "<" + players[it->second].email + "> invite you to join game room " + 
                    players[it->second].inRoomId + ", invitation code is " + rooms[players[it->second].inRoomId]->invitationCode + "\n";
            }
        }
        if(count == 0){
            masterTCPSocket->SendMessage(mes + "No Invitations\n", clientIndex);
            return;
        }
        else
            masterTCPSocket->SendMessage(mes, clientIndex);
    }
    void AcceptInvitation(vector<string> cmds, int clientIndex){
        string email = cmds[1];
        string code = cmds[2];
        if(players[clientIndex].name == ""){
            masterTCPSocket->SendMessage("You are not logged in\n", clientIndex);
            return;
        }
        if(players[clientIndex].inRoomId != ""){
            masterTCPSocket->SendMessage("You are already in game room " +  players[clientIndex].inRoomId + ", please leave game room\n", clientIndex);
            return;
        }
        bool isInvitationValid = false;
        for(auto it = players[clientIndex].invitation.begin(); it != players[clientIndex].invitation.end() ; it++)
        {
            if(players[it->second].email == email && players[it->second].inRoomId == it->first)
            {
                isInvitationValid = true;
                break;
            }
        }
        if(!isInvitationValid){
            masterTCPSocket->SendMessage("Invitation not exist\n", clientIndex);
            return;
        }
        GameRoom* room = nullptr;
        for(auto it = rooms.begin() ; it!= rooms.end(); it++)
        {
            if(it->second->invitationCode == code){
                room = it->second;
                break;
            }
        }
        if(!room)
        {
            masterTCPSocket->SendMessage("Your invitation code is incorrect\n", clientIndex);
            return;
        }
        if(room->isInGame)
        {
            masterTCPSocket->SendMessage("Game has started, you can't join now\n", clientIndex);
            return;
        }    

        masterTCPSocket->Broadcast("Welcome, " + players[clientIndex].name + " to game!\n", room->players, clientIndex);
        masterTCPSocket->SendMessage("You join game room " + room->GameRoomId + "\n", clientIndex);
        room->players.push_back(clientIndex);
        players[clientIndex].inRoomId = room->GameRoomId;
    }
    void StartGame(vector<string> cmds, int clientIndex){
        if(players[clientIndex].name == "") {
			masterTCPSocket->SendMessage("You are not logged in\n", clientIndex);
			return;
		}
        if(players[clientIndex].inRoomId == "")
        {
            masterTCPSocket->SendMessage("You did not join any game room\n", clientIndex);
            return;
        }
        if(cmds.size() == 4 && !isValidGuess(cmds[3])) {
			masterTCPSocket->SendMessage("Please enter 4 digit number with leading zero\n", clientIndex);
			return;
		}
		auto room = rooms[players[clientIndex].inRoomId];
        if(room->GameManager != clientIndex){
            masterTCPSocket->SendMessage("You are not game room manager, you can't start game\n", clientIndex);
            return;
        }
        if(room->isInGame){
            masterTCPSocket->SendMessage("Game has started, you can't start again\n", clientIndex);
            return;
        }
        room->StartGame(cmds[2], cmds.size() == 4 ?cmds[3]:"");
		masterTCPSocket->Broadcast("Game start! Current player is " + players[room->game->getCurrentPlayer()].name + "\n", room->game->players, clientIndex);

        players[clientIndex].isInGame = true;
    }
    void Guess(vector<string> cmds, int clientIndex){
        if(players[clientIndex].name == "") {
			masterTCPSocket->SendMessage("You are not logged in\n", clientIndex);
			return;
		}
        if(players[clientIndex].inRoomId == "")
        {
            masterTCPSocket->SendMessage("You did not join any game room\n", clientIndex);
            return;
        }
		auto room = rooms[players[clientIndex].inRoomId];
        if(!room->isInGame && room->GameManager == clientIndex){
            masterTCPSocket->SendMessage("You are game room manager, please start game first\n", clientIndex);
            return;
        }
        else if(!room->isInGame && room->GameManager != clientIndex){
            masterTCPSocket->SendMessage("Game has not started yet\n", clientIndex);
            return;
        }
        if(!isValidGuess(cmds[1])) {
			masterTCPSocket->SendMessage("Please enter 4 digit number with leading zero\n", clientIndex);
			return;
		}
        if(room->game->getCurrentPlayer() != clientIndex)
        {
            masterTCPSocket->SendMessage("Please wait..., current player is " + players[room->game->getCurrentPlayer()].name + "\n",clientIndex);
            return;
        }
        string result = room->game->Guess(cmds[1]);
        if(!room->game->isEnded)
            masterTCPSocket->Broadcast(players[clientIndex].name + " guess \'" + cmds[1] + "\' and got \'" + result + "\'\n", room->game->players, clientIndex);
        else{
            if(result == "Bingo"){
                string mes = players[clientIndex].name + " guess \'" + cmds[1] + "\' and got " + result + "!!! " + players[clientIndex].name + " wins the game, game ends\n";
                masterTCPSocket->SendMessage(mes, clientIndex);
                room->game->players.erase(find(room->game->players.begin(), room->game->players.end(), clientIndex));
                masterTCPSocket->Broadcast(mes, room->game->players, clientIndex);
            }
            else{
                string mes = players[clientIndex].name + " guess \'" + cmds[1] + "\' and got \'" + result + "\'\nGame ends, no one wins\n";
                masterTCPSocket->SendMessage(mes, clientIndex);
                room->game->players.erase(find(room->game->players.begin(), room->game->players.end(), clientIndex));
                masterTCPSocket->Broadcast(mes, room->game->players, clientIndex);
            }
            room->QuitGame();
        }
    }
    void LeaveRoom(int clientIndex){
        if(players[clientIndex].name == "")
        {
            masterTCPSocket->SendMessage("You are not logged in\n", clientIndex);
            return;
        }
        if(players[clientIndex].inRoomId == "")
        {
            masterTCPSocket->SendMessage("You did not join any game room\n", clientIndex);
            return;
        }
        
        auto room = rooms[players[clientIndex].inRoomId];
        if(find(room->players.begin(), room->players.end(), clientIndex) == room->players.end()){
                cout<<"In " + players[clientIndex].inRoomId + "Can't find player\n";
                return;
        }
        room->players.erase(find(room->players.begin(), room->players.end(), clientIndex));
        
        if(room->GameManager == clientIndex){
            masterTCPSocket->SendMessage("You leave game room " + players[clientIndex].inRoomId + "\n", clientIndex);
            rooms.erase(players[clientIndex].inRoomId);
            for(int player : room->players){
                masterTCPSocket->SendMessage("Game room manager leave game room " + players[clientIndex].inRoomId + ", you are forced to leave too\n", player);
                players[player].inRoomId = "";
            }
            players[clientIndex].inRoomId = "";
            return;
        }
        else{
            if(room->isInGame){
                masterTCPSocket->SendMessage("You leave game room " + players[clientIndex].inRoomId + ", game ends\n", clientIndex);
                for(int player : room->players)
                    masterTCPSocket->SendMessage(players[clientIndex].name + " leave game room " + players[clientIndex].inRoomId + ", game ends\n", player);
                
                players[clientIndex].inRoomId = "";
                room->QuitGame();
                return;
            }
            else{
                masterTCPSocket->SendMessage("You leave game room " + players[clientIndex].inRoomId + "\n", clientIndex);
                for(int player : room->players)
                    masterTCPSocket->SendMessage(players[clientIndex].name + " leave game room " + players[clientIndex].inRoomId + "\n", player);
                
                players[clientIndex].inRoomId = "";
                return;
            }
        }
    }
    void Exit(int clientIndex){
        if(players[clientIndex].inRoomId != "")
        {
            auto room = rooms[players[clientIndex].inRoomId];
            room->players.erase(find(room->players.begin(), room->players.end(), clientIndex));
        
            if(room->GameManager == clientIndex){
                rooms.erase(players[clientIndex].inRoomId);
                for(int player : room->players)
                    players[player].inRoomId = "";
            }
            else if(room->isInGame)
                room->QuitGame();
        }
        players[clientIndex].Clear();
        masterTCPSocket->Close(clientIndex);
    }
    void TCP(){
        //set master socket to allow multiple connections
        int opt = 1;
        //create a master socket 
        masterTCPSocket = new TCPProtocol();
        if(setsockopt(masterTCPSocket->tcpSockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, (char *)&opt, sizeof(opt)) < 0 )  
        {  
            perror("setsockopt");  
            exit(EXIT_FAILURE);  
        }
        masterTCPSocket->SetServerAddress(&serverAddress);
        masterTCPSocket->Bind();
        masterTCPSocket->Listen();
    }
    void UDP(){
        uProtocol;
        uProtocol.SetServerAddress(&serverAddress);
        uProtocol.Bind();       
    }
    vector<string> SplitCommand(string cmd){
        vector<string> cmds;
        string tmp = "";
        for(auto c : cmd)
        {
            if(c == ' ')
            {
                cmds.push_back(tmp);
                tmp = "";
            }
            else{
                if((int)c < 32) // Specific
                    continue;
                tmp.push_back(c);
            }
        }
        cmds.push_back(tmp);
        return cmds;
    }
    bool isValidGuess(string port){
        if(port.size() != 4)
            return false;

        for(int i = 0 ;i< 4;i++)
        {
            if((int)port[i] < '0' || (int)port[i] > '9')
            return false;
        }
        return true;
    }
    fd_set rset;
    int nready;

    UDPProtocol uProtocol;
    TCPProtocol* masterTCPSocket;
    struct sockaddr_in serverAddress={0};
    map<string, pair<string, string> > user2data;
    map<string, pair<string, string> > email2data;
    map<string, GameRoom*> rooms;
    playerInfo players[MAX_CLIENT + 3];
};

int main(int argc, char** argv) {
    int port = 8888;
    if(argc == 2){
        port = atoi(argv[1]);
    }
    Server* server = new Server(port);

    server->LogicLoop();

    return 0;
}