#include "osc.h"
#include <string>
#include <sstream>
#include <iostream> //rkh
#include "osc/OscOutboundPacketStream.h"
#include "ip/UdpSocket.h"
#include "ip/IpEndpointName.h"
#include "osc/OscReceivedElements.h"
#include "osc/OscPacketListener.h"
#define ADDRESS "localhost"
#define PORT 57120

// for readIni method
#include <fstream>
#include <vector>

std::string osc_targets[10][2];
std::vector<std::string> osc_ips;
std::vector<std::string> osc_ports;

int enabled = 0;

namespace oscplugin {
	//osc_input_vars gOSCvars;
	
	
	// if dynamic IP is enabled, ip's will be selected from an array populated by the user
	// currently, ips are set in the client and assed in
	void getIPAddress (int index) {
		std::cout << "testing OSC library" << std::endl;
	}
	
#define OUTPUT_BUFFER_SIZE 2048
	
	void init() {
		
		// ADD CALL TO INIT in BulletSystem::initialize
		using namespace std;
		cout << "____ CCRMA/MiTo: read target IP Address list from osc.csv (osc.cpp) ___" << endl;        
		ifstream inFile ("osc.csv");
		string line;
		int linenum = 0;
		
		while (getline (inFile, line)) {
			
			istringstream linestream(line);
			string item;
			int itemnum = 0;
			
			while (getline (linestream, item, ',')) {
				
				if(itemnum==0) {
					osc_ips.push_back(item);
				} else {
					osc_ports.push_back(item);					
				}
				
				osc_targets[linenum][itemnum] = item;
				itemnum++;				
			}
			
			linenum++;
		}
/*		
		for(int i=0;i<linenum;i++) {
			cout << "Ip Address:Port => " << osc_targets[i][0] << ":" << osc_targets[i][1] << endl;
			cout << "Ip Address:Port => " << osc_ips[i] << ":" << osc_ports[i] << endl;			
		}
*/		
        if (osc_ips[0] != "0.0.0.0") {
            enabled = 1;
        }
    }
	
	int isActive() {
		return enabled;	
	}
	
    void sendOSCmessage(mito_data data) {
        using namespace std;
        if (enabled) {
        	
            /*
             std::string s="hello";
             const char *p = s.c_str(); // get const char * representation
             int len = strlen(p);
             */
            int size = osc_ips.size();
    
            for (int i=0;i<size;i++) {
    //   cout << "sendOSCmessage to => " << osc_targets[i][0] << ":" << osc_targets[i][1] << endl;
    //  cout << "sendOSCmessage vector => " << (std::string)osc_ips[0] << ":" << (std::string)osc_ports[0] << endl;
    //  cout << "sendOSCmessage to => " << osc_targets[0][0] << ":" << osc_targets[0][1] << endl;
    
    
    //   cout << "sendOSCmessage vector => " << (std::string)osc_ips[i] << ":" << (std::string)osc_ports[i] << endl;
    
                //  string ip(osc_targets[0][0]);
                //  string port(osc_targets[0][1]);
    
                //  string ip(osc_targets[i][0]);
                //  string port(osc_targets[i][1]);
    
                //  if(strlen(osc_targets[i][0])>0 && strlen(osc_targets[i][1]>0) {
    
                char buffer[OUTPUT_BUFFER_SIZE];
                osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );
                //      UdpTransmitSocket socket( IpEndpointName( ADDRESS, PORT ));
                //   UdpTransmitSocket socket( IpEndpointName( osc_targets[i][0], osc_targets[i][1] ));
    //   UdpTransmitSocket socket( IpEndpointName( osc_targets[0][0].c_str(), atoi(osc_targets[0][1].c_str()) ));
                UdpTransmitSocket socket( IpEndpointName( osc_ips[i].c_str(), atoi(osc_ports[i].c_str()) ));
                p.Clear();
    
    
                p << osc::BeginMessage( "/mito" )
                << data.user_id
                << data.path_id
                << data.cell_id
                << data.global_x
                << data.global_y
                << data.global_z
                << data.relative_x
                << data.relative_y
                << osc::EndMessage;
    
                if (p.IsReady()) {
                    socket.Send( p.Data(), p.Size() );
                }
            }
        }
        // }
    }
	
	/*
	 void sendOSCbundle(ball_coordinates currentClient) {
	 
	 std::string str(currentClient.port);
	 std::istringstream strin(str);
	 int port;
	 strin >> port;
	 
	 char buffer[OUTPUT_BUFFER_SIZE];
	 osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );
	 UdpTransmitSocket socket( IpEndpointName( currentClient.hostname, port ));
	 p.Clear();
	 
	 p << osc::BeginBundle()
	 << osc::BeginMessage( "/x" ) << currentClient.origin[0] << osc::EndMessage
	 << osc::BeginMessage( "/y" ) << currentClient.origin[1] << osc::EndMessage
	 << osc::BeginMessage( "/z" ) << currentClient.origin[2] << osc::EndMessage
	 << osc::EndBundle;
	 
	 if (p.IsReady()) {
	 socket.Send( p.Data(), p.Size() );
	 }
	 }
	 */
}

