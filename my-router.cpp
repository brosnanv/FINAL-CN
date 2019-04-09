#include <arpa/inet.h>
#include <errno.h> 
#include <netinet/in.h> 
#include <signal.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <strings.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <iostream>
#include <thread>
#include <chrono>
#include<vector>
#include <fstream> 
#include <string.h>
#include<time.h>
#include<ctime>

using namespace std;

#define MAXLINE 1600 
#define MAX_ROUTERS 6

struct sockaddr_in cliaddr, servaddr;
int udpfd;
int removed=6;
string createDV();
int insertEdge(struct graph* g, char src, char dest, int cost);
void BellmanFord(struct graph* g, int src);
void writedvtable(char src);
void handlePacket(char buffer[MAXLINE]);
void printPath(int parent[], int j);
int findNext(int destP);
void updateTime(char node);
void removeNode(int node);
void reAppear(char dis, int weight);
void printdv(char dv[], char src);
void forwardingtable(char src);
void printtime(char src);
void graphDelete (graph* g);

// Some sort of structure or class will be needed to keep details about 
// immediate neighbours

// Struct to hold router details
struct node {
	char letter;
	int pNum;  // port number 
	int numConnec; // number of neighbours it is connected to 
  // sockaddr_in addr; 
} node1;


// Details of neighbour routers
struct neighbourNode {
	char letter;
	int pNum;
	int cost;
	time_t time; 
}neighbour[MAX_ROUTERS];

struct edge {
	char src;
	char dest;
	int cost;
};

struct graph {
	int V, E;
	vector<struct edge*> edges;
};

struct router
{
	char src;
	int portnum;
};

//struct graph* maingraph;
graph *g = (struct graph*)malloc(sizeof(struct graph));

void initGraph()
{
	//struct graph* g = new graph;
	g->E = 0;
	g->V = 0;
	g->edges.resize(0);
	//return g;
}

void cleargraph(graph *g)
{
	vector<struct edge*>().swap(g->edges);
}
void graphDelete(graph *g)
{
	//cout << "\n! ! ! ***** In graph delete  *****! ! ! \n  "; 
	for (int i = 0; i < g->edges.size(); i++) {

		free(g->edges[i]);
		// >src << " Ending node:C" << g->edges[i]->dest << " Weight " << g->edges[i]->cost << endl;
	}

	 g->edges.resize(0);
	// cout<< "After delete: "<< g->edges.size() <<endl;



}
struct DV {
	char node;
	int min_dist;
	char nextNode;
}dvtable[MAX_ROUTERS];


int converge = 0;
void parseReceived(char buff[MAXLINE]) {
	// In this function we will take what was received
	// in buffer and parse the details.
	// It will need to check if to see if there has been a change- call add edge 

	string source, dest, tempCost;
	fstream file;
	char sor, des;
	int cost;
	int nochange = 0;
	string s(buff);

	int curr = s.find("(");
	s.erase(0, curr + 1);

	if (buff[0] == 'D') {
		for (int count = 0; count < MAX_ROUTERS; count++) {

			// take in the source 
			curr = s.find(",");
			source = s.substr(0, curr);
			s.erase(0, curr + 1);
			//take in the destination 
			curr = s.find(",");
			dest = s.substr(0, curr);
			s.erase(0, curr + 1);
			// read in the cost 
			curr = s.find(")");
			tempCost = s.substr(0, curr);
			cost = stoi(tempCost);
			s.erase(0, curr + 1);

			sor = source[0];
			// update struct time
			updateTime(sor);
			reAppear(sor,cost); // check if a reappearing node- only receive message from direct neighbours

			des = dest[0];
			// reset to after next bracket 
			curr = s.find("(");
			s.erase(0, curr + 1);
			// test parse received
			//cout<<count <<"Source:"<<source <<" dest "<<dest <<"Cost "<< cost <<endl;
			//timestamp
			
			// output previous routing table here: 
	

			if (insertEdge(g, sor, des, cost) == 1) {
				nochange++;
			}
		}
		//cout<<"No change: "<<nochange<<endl;
		if (nochange != removed) {
		//	cout<<"Remoed:"<<removed;
			printtime(node1.letter);
			forwardingtable(node1.letter);
			BellmanFord(g, node1.letter);
			//Will need to write these to file 
			//cout << buff << endl; create function writeDV(buff)
			printdv(buff,node1.letter);
			forwardingtable(node1.letter);
			//writedvtable(node1.lett);
			
		}
		//if(nochange==removed){
		//	converge=0;
		//}
		else {
			if (converge != -1) {
				cout << "Enter Packet Destination: " << endl;
				converge = -1;
	
			}
		}
	}
	else {

		handlePacket(buff);
	}
}

void reAppear(char dis, int weight) {
	char a;
	int x = 0;
	int port1 = dis + 9935;
	for (int i = 0; i < node1.numConnec; i++) {
		if (dis != neighbour[i].letter) {
			x++;
		}
	}
	if (x == node1.numConnec) {
		cout << "New neighbour appeared: " << dis << endl;
		node1.numConnec = node1.numConnec + 1;
		neighbour[node1.numConnec].letter = dis;
		neighbour[node1.numConnec].cost = weight;
		neighbour[node1.numConnec].pNum = port1;
		time(&neighbour[node1.numConnec].time);
		graphDelete(g);

		initGraph();

	for (int i = 0; i < node1.numConnec; i++) {
		insertEdge(g, node1.letter, neighbour[i].letter, neighbour[i].cost);
		time(&neighbour[i].time);
	}
 
	converge=0; 
	char rem[]="Node Added";
	
	printdv(rem,node1.letter);

	BellmanFord(g, node1.letter);
	}


}

void updateTime(char node) {
	char a;
	for (int i = 0; i < node1.numConnec; i++) {
		if (node==neighbour[i].letter) {
			a = neighbour[i].letter;
			time(&neighbour[i].time);
		}
	}
	//cout << "Updated time for: " << a << endl;
	

}

void checkTime_th() {
	for (;;) {
		struct timeval t;
		t.tv_sec = 5;
		select(0, NULL, NULL, NULL, &t);
		time_t time_1;
		time(&time_1);
		for (int i = 0; i < node1.numConnec; i++) {
			if ((time_1 - neighbour[i].time) > 25) {
				cout << "Router " << neighbour[i].letter << " No longer available"<<endl;
				removeNode(i);
			}
		}
	}
}

void removeNode(int node) {
	
	for (int j = node; j < MAX_ROUTERS; ++j) {
		neighbour[j] = neighbour[j + 1];
	}

	node1.numConnec = node1.numConnec - 1;

	//	cleargraph(g);

	graphDelete(g);

	initGraph();


	for (int i = 0; i < node1.numConnec; i++) {
		insertEdge(g, node1.letter, neighbour[i].letter, neighbour[i].cost);
		time(&neighbour[i].time);
	}

	converge=0; 
	char rem[]="Node Removed";
	printdv(rem,node1.letter);

	BellmanFord(g, node1.letter);
	//forwardingtable(node1.letter);
	//removed = removed-1 ;
//	cout <<"test- past bellman ford \n \n ";
}


// This function is continuously checking for information on socket
void receive_th() {

	int nready, maxfdp1;
	char buffer[MAXLINE];
	ssize_t n;
	fd_set rset;
	socklen_t len;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //(INADDR_ANY);
	servaddr.sin_port = htons(node1.pNum);

	udpfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (bind(udpfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
		perror("Bind");
	}

	for (;;) {

		struct timeval t;
		t.tv_sec = 5;
		t.tv_usec = 0;

		// binding server addr structure to udp sockfd 


		// clear the descriptor set 
		FD_ZERO(&rset);

		// get maxfd 
		maxfdp1 = udpfd;
		FD_SET(udpfd, &rset);

		if ((nready = select(maxfdp1 + 1, &rset, NULL, NULL, &t)) == -1) {
			perror("select");
		}

		// if udp socket is readable receive the message. 
		if (FD_ISSET(udpfd, &rset)) {
			len = sizeof(cliaddr);
			bzero(buffer, sizeof(buffer));


			//printf("\nMessage from Router: ");
			n = recvfrom(udpfd, buffer, sizeof(buffer), 0,
				(struct sockaddr*)&cliaddr, &len);
			//puts(buffer);
			parseReceived(buffer);
		}
	}
}


void injectPacket_th() {
	int pckDest;
	string packet;
	string body ="Hello i am a packet";
	string header = "PCK ";
	string des = "Dest:";
	string newl = "\n";
for(;;){

	cout<<"Enter packet dest: "<<endl;
	cin >> pckDest;

	packet = header + des + to_string(pckDest) + newl + body;
	cout << packet << endl;
	
	// Need to search DV struct for next node for pck dest
	int nextID;
	nextID = findNext(pckDest);
	
	
	// Send to next hope node
	cliaddr.sin_port = htons(nextID);
	sendto(udpfd, packet.c_str(), packet.length(), 0,
		(struct sockaddr*)&cliaddr, sizeof(cliaddr));
}

}

void handlePacket(char buffer[MAXLINE]) {
	// Get DV up and running before tackling this 
	// Will parse header to find destination router
	// Use nextHopTable to find port to send the packet
	// Send packet to this port 

	//forwards to the next node-parse to find destination
	int cur;
	string destPort;

	string p(buffer);
	string forward(buffer);
	cur = p.find(":");
	p.erase(0, cur+1);
	cur = p.find("\n");
	destPort = p.substr(0, cur);
	cout << "Packet destination: " << destPort << endl;
	//cout <<"This is in buffer\n"<< buffer << endl;
	int dPort = stoi(destPort);
	
	if (dPort == node1.pNum) {
		//cout << "Packet arrived at dest: " << buffer << endl;
		string psend2 = "Packet has arrived: " + string(buffer);
		char send2[150];
		strcpy(send2,psend2.c_str());
		printdv(send2,node1.letter);

	}
	else {
		int nextNode = findNext(dPort);
		cliaddr.sin_port = htons(nextNode);
		sendto(udpfd, forward.c_str(), forward.length(), 0,
			(struct sockaddr*)&cliaddr, sizeof(cliaddr));
		cout << "Packet sent to port: "<<dPort<<  endl;
		string psend = "Packet sent to port: " + destPort;
		char send1[50];
		strcpy(send1,psend.c_str());
		printdv(send1,node1.letter);
	}

}

void send_th() {

	const char* message = "Hello Router";
	for (;;) {

		struct timeval t;
		t.tv_sec = 5;
		t.tv_usec = 0;
		select(0, NULL, NULL, NULL, &t);

		// Here we will loop through our direct neighbours sending them a DV 
		// every 5s 
		string message;

		message = createDV();

		// Send DV

		for (int i = 0; i < node1.numConnec; i++) {
			cliaddr.sin_port = htons(neighbour[i].pNum);
			sendto(udpfd, message.c_str(), message.length(), 0,
				(struct sockaddr*)&cliaddr, sizeof(cliaddr));
			cout << "Sent message to port: " << neighbour[i].pNum <<endl;
			/* code */// endl;

		}


	}

}

void parseTopology(char* file) {
	// From topology file we are only interested in direct neigbours/* code */
	// We will receive other neighbours from exchanging DVs
	// If sub string 0 is equal to A then parse that line etc
	// i.e. parse only lies beginning with 'A'
	// Store information in struct or class 

	char letter, dest;
	int portNum;
	int cost;
	char buffer[MAXLINE];
	char createdRouters[MAX_ROUTERS];// not sure if needed

	ifstream tableFile;
	tableFile.open(file);

	if (!tableFile) {
		cout << " Fail to open table topology file \n";
	}
	// reads in first line which is description
	tableFile.getline(buffer, MAXLINE);

	if (tableFile.is_open()) {


		while (tableFile) {
			tableFile >> letter;

			if (node1.letter == letter) {
				// if its A parse a and add 1 to number connections
				node1.numConnec++;
				tableFile >> dest >> portNum >> cost;

				neighbour[node1.numConnec - 1].letter = dest; 
				neighbour[node1.numConnec - 1].pNum = portNum;
				neighbour[node1.numConnec - 1].cost = cost;
			}
			else {
				tableFile.getline(buffer, MAXLINE);
				memset(buffer, 0, MAXLINE);
			}
			letter = '0';

		}

	}
	cout << "\nThe number of directly connected neighbours is :" << node1.numConnec << endl<< "Direct neighbours:\n";
	tableFile.close();

}

string createDV() {
	// Use information from routing table to create DV
	// Take the row correlating to that router 
	// Format of DV in Report 
	// Single vector containing distances to all neighbour nodes
	string DV = "DV ";
	string bracket = "(";
	string bracket1 = ")";
	string comma = ",";
	for (int i = 0; i < MAX_ROUTERS; i++) {

		DV = DV + bracket + node1.letter + comma + dvtable[i].node + comma + to_string(dvtable[i].min_dist) + bracket1;

	}
	//cout << DV;

	return DV;


}


void BellmanFord(struct graph* g, int src)
{
	int V = g->V;
	int E = g->E;
	int next;

	// Step 1: Initialize distances from src to all other vertices
	// as INFINITE

	for (int i = 0; i < MAX_ROUTERS; i++)
	{
		dvtable[i].node = (char)(i + 65);
		dvtable[i].min_dist = 10000;
		dvtable[i].nextNode = -1;
	}

	//Assuming A is first vertex and so on...
	dvtable[src % 65].min_dist = 0;

	// Step 2: Relax all edges |V| - 1 times.
	for (int i = 1; i <= V - 1; i++)
	{

		for (int j = 0; j < E; j++)
		{
		
			int u = g->edges[j]->src;
			int v = g->edges[j]->dest;
			int cost = g->edges[j]->cost;
			//if (dist[u-65] != 10000 && dist[u-65] + cost < dist[v-65])
			//    dist[v-65] = dist[u-65] + cost;
			if (dvtable[u % 65].min_dist != 10000 && dvtable[u % 65].min_dist + cost < dvtable[v % 65].min_dist) {
				dvtable[v % 65].min_dist = dvtable[u % 65].min_dist + cost;
				dvtable[v%65].nextNode = dvtable[u%65].node;

			}
		}
	}
	
}

int findNext(int destP) {
	// ROUTES THROUGH ALL PREVIous to find next node 

	int port = destP - 10000;
	char a, b;
	b = dvtable[port].nextNode;
	if (b == node1.letter) {
		a = dvtable[port].node;
	}
	else {
		while (a != node1.letter) {
			a = dvtable[b % 65].nextNode;
			if (a != node1.letter) {
				b = a;
			}
		}
		a = dvtable[b % 65].node;
	}
	cout << "Next Node: " << a << endl;
	return (a + 9935);
}

void printPath(int parent[], int j)
{
	// Base Case : If j is source 
	if (parent[j] == -1)
		return;

	printPath(parent, parent[j]);

	printf("%d ", j);
	cout << endl;
}


int insertEdge(struct graph* g, char src, char dest, int cost) {
	int flag = 0;
	edge *e = (struct edge*)malloc(sizeof(struct edge));
	e->src = src;
	e->dest = dest;
	e->cost = cost;
	if (g->V == 0)	//if this is the first edge of our setup
	{
		g->E++;
		g->V += 2;
		g->edges.push_back(e);
		//cout << "Properties of Added Edge: " << g->edges[0]->src << "->" << g->edges[0]->dest << " cost: " << g->edges[0]->cost;
	}
	else {
		for (int i = 0; i < g->edges.size(); i++) {     //If edge already exists, just update cost
			if (g->edges[i]->src == src) {
				if (g->edges[i]->dest == dest) {
					//cout << "Edge already exists" << endl;
					flag = 1;
					if (g->edges[i]->cost != cost && g->edges[i]->cost<15) {
						//cout << "Updating cost" << endl;
						g->edges[i]->cost = cost;
						flag = 2;
					}
					else if(g->edges[i]->cost>=30&&g->edges[i]->dest == dest)
					{
						g->edges[i]->cost=10000;
					
						flag=1; 
					}
					
					
				}
			}
		}
		if (flag == 0) {     //Adding edge if it doesn't already exist in the graph
			int index, v1 = 1, v2 = 1;
			g->E++;
			g->edges.push_back(e);
			for (int i = 0; i < g->edges.size(); i++)	//seeing if vertices have been previously defined
			{
				if (g->edges[i]->src == src || g->edges[i]->dest == dest)
					v1 = 0;
				if (g->edges[i]->src == dest || g->edges[i]->dest == src)
					v2 = 0;
			}
			g->V = g->V + v1 + v2;
			index = g->edges.size() - 1;
		//	cout << "Inserted new edge***:" << g->edges[index]->src << "->" << g->edges[index]->dest << " W: " << g->edges[index]->cost << endl;
		}
	}
	//	cout << "\n";

	return flag;
}

void clearfile(char src)
{
	ofstream file;
	string node(1,src);
	string createtable = "Routing-output" + node + ".txt";
	file.open(createtable, ios::out|ios::trunc);
	file.close();
}

void forwardingtable(char src)
{
	ofstream file;
	char base = 'A';
	string node(1,src);
	string createtable = "Routing-output" + node + ".txt";
	file.open(createtable, ios::app);
	file << "Destination" << "\t\t" << "Cost" << "\t\t" << "Outgoing UDP Port" << "\t\t" << "Destination UDP Port \n";
	for (int i = 0; i < MAX_ROUTERS; i++)
	{
		if (dvtable[i].node != src && dvtable[i].min_dist<15){
			file << dvtable[i].node << "\t\t\t\t" << dvtable[i].min_dist << "\t\t\t\t"<<10000 + int(src) - int(base) <<"\t\t\t\t\t\t" <<10000 + int(dvtable[i].node) - int(base) << "\n";
		}
	}
	
	file.close();
}

void printtime(char src)
{
	ofstream file;
	string node(1,src);
	string createtable = "Routing-output" + node + ".txt";
	file.open(createtable, ios::app);
	file<<"\n";
	time_t my_time = time(NULL); 
  file<<"Time is "<<ctime(&my_time); 
	file.close();
}

void printdv(char dv[],char src)
{
	ofstream file;
	string buf(dv);
	string node(1,src);
	string createtable = "Routing-output" + node + ".txt";
	file.open(createtable, ios::app);
	file<<buf<<"\n";
	file.close();
}

void writedvtable(char src)
{
	ofstream file;
	char fail='N';
	string node(1,src);
	string createtable = ("Routing-output"+node+".txt");
	file.open(createtable,ios::out);
	file << "ROUTER"<<"\t\t\t"<<"SHORTEST_DISTANCE"<<"\t\t\t\t"<<"PREV_NODE\n";
	for (int i = 0; i < MAX_ROUTERS; i++) {
		if (dvtable[i].node != -1) {
			if(dvtable[i].min_dist==10000)
				dvtable[i].nextNode=fail;
			else if (dvtable[i].node == src)
				file << dvtable[i].node << "\t\t\t\t" << dvtable[i].min_dist << "\t\t\t\t*\n";
			else
				file << dvtable[i].node << "\t\t\t\t" << dvtable[i].min_dist << "\t\t\t\t\t\t\t\t" << dvtable[i].nextNode << "\n";
		}
	}
	file.close();
}

void printgraph(struct graph* g) {

	for (int i = 0; i < g->edges.size(); i++) {
		cout << "Starting node: " << g->edges[i]->src << " Ending node:C" << g->edges[i]->dest << " Weight " << g->edges[i]->cost << endl;
	}
}

int main(int argc, char *argv[])
{
	char *filename;
	char r_name;

	if (argc > 1) {
		//Check if a file has been entered as argument 

		if (argv[1]) {
			filename = argv[1];

		}

		if (argv[2]) {
			// The second argument will contain the id of the router 
			// we are setting up. i.e. A or B or C ....
			node1.letter = argv[2][0];
		}
	}
	//cout<< "At beginning: "<< g->edges.size() <<endl;
	// Convert letter to int port number 
	node1.pNum = node1.letter + 9935;
	cout << "Port Number: " << node1.pNum << endl;
	clearfile(node1.letter);

	//Parse topology file
	parseTopology(filename);
	
	initGraph();

	for (int i = 0; i < node1.numConnec; i++) {
		insertEdge(g, node1.letter, neighbour[i].letter, neighbour[i].cost);
		time(&neighbour[i].time);
	}

	//printgraph(g);

	BellmanFord(g, node1.letter);



	for (int i = 0; i < MAX_ROUTERS; i++) {
		if (dvtable[i].min_dist !=10000 && dvtable[i].min_dist !=0 ){
		cout << "		 Node " << dvtable[i].node << " Dist: " << dvtable[i].min_dist << endl; 
		//" Next: " << dvtable[i].nextNode << endl;
		}
	}


	// Set up thread so send and receive can run async
	thread recv(receive_th);
	thread dispatch(send_th);
	thread check(checkTime_th);
	thread inject(injectPacket_th);
	
	// Wait until
	dispatch.join();
	recv.join();
	check.join();
	inject.join();

}