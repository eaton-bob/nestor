/*  =========================================================================
    fty_exdialogue_server - Actor Server

    Copyright (C) 2014 - 2017 Eaton                                        
                                                                           
    This program is free software; you can redistribute it and/or modify   
    it under the terms of the GNU General Public License as published by   
    the Free Software Foundation; either version 2 of the License, or      
    (at your option) any later version.                                    
                                                                           
    This program is distributed in the hope that it will be useful,        
    but WITHOUT ANY WARRANTY; without even the implied warranty of         
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          
    GNU General Public License for more details.                           
                                                                           
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.            
    =========================================================================
*/

/*
@header
    fty_exdialogue_server - Actor Server
@discuss
@end
*/
        
#include <vector>
#include <iosfwd>
#include <string>

#include "fty_exdialogue_classes.h"

//handle mailboxes
static void
s_handle_mailbox (mlm_client_t *client, zmsg_t **message, std::string serverName)
{
    char *part = NULL, *cName = NULL;

    assert(client);
    assert(message);
    if (!(client && message))
        return;

    //get clientName from msg
    cName = zmsg_popstr(*message);
    if (!cName ) {
        zsys_warning("SERVER cName is null");

        zmsg_t *reply = zmsg_new ();
        zmsg_addstr(reply, "ERROR");

        int rv = mlm_client_sendto (client, mlm_client_sender (client), "example", NULL, 1000, &reply);
        if (rv != 0) {
            zmsg_destroy(&reply);
            zsys_error (
                    "SERVER mlm_client_sendto() failed (sender = '%s', subject = '%s', timeout = '1000').",
                    mlm_client_sender (client), "example");
        }

        goto DONE;
    } 
    
    //get discussion part from msg
    part = zmsg_popstr(*message);
    if (!part) {
        zsys_warning("SERVER part is null");

        zmsg_t *reply = zmsg_new ();
        zmsg_addstr(reply, "ERROR");

        int rv = mlm_client_sendto (client, mlm_client_sender (client), "example", NULL, 1000, &reply);
        if (rv != 0) {
            zmsg_destroy(&reply);
            zsys_error (
                    "SERVER mlm_client_sendto() failed (sender = '%s', subject = '%s', timeout = '1000')",
                    mlm_client_sender (client), "example");
        }

        goto DONE;
    }

    zsys_debug("SERVER %s say : %s", cName, part);

    if (streq (part, "Hello !")) {
        std::string answer = "Hello " + std::string(cName) + ".";

        zmsg_t *reply = zmsg_new ();
        zmsg_addstr(reply, "Stranger");
        zmsg_addstr(reply, answer.c_str());

        int rv = mlm_client_sendto (client, mlm_client_sender (client), "example", NULL, 1000, &reply);
        if (rv != 0) {
            zmsg_destroy(&reply);
            zsys_error (
                    "SERVER mlm_client_sendto() failed (sender = '%s', subject = '%s', timeout = '1000')",
                    mlm_client_sender (client), "example");
        } 
    } else if(streq (part, "Who are you ?")) {
        std::string answer = "My name is " + serverName;

        zmsg_t *reply = zmsg_new();
        zmsg_addstr (reply, serverName.c_str());
        zmsg_addstr (reply, answer.c_str());
                
        int rv = mlm_client_sendto(client, mlm_client_sender(client), "example", NULL, 1000, &reply);
        if(rv != 0){
            zmsg_destroy(&reply);
            zsys_error (
                    "SERVER mlm_client_sendto() failed (sender = '%s', subject = '%s', timeout = '1000')",
                    mlm_client_sender (client), "example");
        }
    } else if(streq (part, std::string("Good bye " + serverName + " !").c_str())) {
        std::string answer = "Good bye " + std::string(cName) + ".";

        zmsg_t *reply = zmsg_new();
        zmsg_addstr(reply, serverName.c_str());
        zmsg_addstr(reply, answer.c_str());
        
        int rv = mlm_client_sendto(client, mlm_client_sender(client), "example", NULL, 1000, &reply);
        if(rv != 0){
            zmsg_destroy(&reply);
            zsys_error (
                    "SERVER mlm_client_sendto() failed (sender = '%s', subject = '%s', timeout = '1000')",
                    mlm_client_sender (client), "example");
        }
    } else { 
        zmsg_t *reply  = zmsg_new ();
        zmsg_addstr(reply, "ERROR");

        int rv = mlm_client_sendto (client, mlm_client_sender (client), "example", NULL, 1000, &reply);
        if (rv != 0) {
            zmsg_destroy(&reply);
            zsys_error (
                    "SERVER mlm_client_sendto() failed (sender = '%s', subject = '%s', timeout = '1000')",
                    mlm_client_sender (client), "example");
        }
    }

DONE:
    //cleanup
    zstr_free(&part);
    zstr_free(&cName);
    zmsg_destroy(message);
}

// server callback function
void
fty_exdialogue_server (zsock_t *pipe, void* args)
{
    int verbose = 1; //default
    
    std::vector<std::string> arguments = *(std::vector<std::string> *) args;
    std::string serverName = arguments[0];

    zsys_info("SERVER %s starting...", serverName.c_str());

    //create malamute client
    mlm_client_t *client = mlm_client_new();
    if (!client) {
        zsys_error("SERVER mlm_client_new() failed.");
        return;
    }

    //create poller on main pipe and msg pipe
    zpoller_t *poller = zpoller_new (pipe, mlm_client_msgpipe(client), NULL);
    zsock_signal (pipe, 0); //SIG needed (wakeup)

    zsys_debug("SERVER %s ready", serverName.c_str());

    //start polling
    const int TIMEOUT = 5000; //ms
    int stop = 0; //internal abort flag
    
    while (!zsys_interrupted && !stop) {
        //wait for inputs
        void *sock = zpoller_wait(poller, -1);

        if (sock == pipe) {
            zmsg_t *message = zmsg_recv(pipe);
            char *command = zmsg_popstr(message);
            assert(message);
            assert(command);

            //$TERM command implementation is required by zactor_t interface
            if (streq (command, "$TERM")) {
               zsys_debug("SERVER %s %s", serverName.c_str(), command);
               stop = 1; //abort
            }//$TERM
 
            //BIND command
            if (streq (command, "BIND")) {
                char *endpoint = zmsg_popstr(message);
                char *address = zmsg_popstr(message); //address
                assert(endpoint);
                assert(address);
                zsys_debug("SERVER %s %s %s %s", serverName.c_str(), command, endpoint, address);

                int rv = mlm_client_connect(client, endpoint, TIMEOUT, address);
                if (rv != 0) {
                    stop = 1; //abort
                    zsys_error(
                        "SERVER mlm_client_connect() failed (endpoint = %s, address = %s)",
                        endpoint, address);
                }

                //cleanup
                zstr_free(&endpoint);
                zstr_free(&address);                    
            }//BIND

            //VERBOSE command
            if (streq (command, "VERBOSE")) {
                char *param = zmsg_popstr(message); //'true'/'false'
                assert(param);
                zsys_debug("SERVER %s %s %s", serverName.c_str(), command, param);

                verbose = streq(param, "true") ? 1 : 0;
 
                //cleanup
                zstr_free(&param);                    
            }
            
            //cleanup
            zstr_free(&command);
            zmsg_destroy(&message);

            continue;
        }//main pipe

        if (sock == mlm_client_msgpipe(client)) {
            zmsg_t *message = mlm_client_recv(client);
            assert(message);

            if (!message) {
                stop = 1; //abort
                zsys_error("SERVER msg pipe interrupted");
            }
            else if (streq (mlm_client_command (client), "MAILBOX DELIVER")) {
                //handle mailbox msg
                s_handle_mailbox(client, &message, serverName);
            }
            else {
                zsys_warning (
                    "SERVER Unsupported malamute pattern: '%s'. Message subject: '%s', sender: '%s'.",
                    mlm_client_command(client), mlm_client_subject(client), mlm_client_sender(client));
            }
 
            //cleanup
            zmsg_destroy(&message);
            
            continue;
        }//msg pipe
    }

    zsys_info("SERVER %s ended", serverName.c_str());

    //cleanup
    mlm_client_destroy(&client);
    zpoller_destroy(&poller);
}

//  --------------------------------------------------------------------------
//  Self test of this class
//  NOTE: test the whole communication scheme beetween server and client,
//  then the client selftest fty_exdialogue_client_test() is empty.

void
fty_exdialogue_server_test (bool verbose)
{
    int rv;
    
    // @selftest
    zsys_info ("fty_exdialogue_server_test started");
    
    static const char* endpoint = "inproc://fty-exdialogue-server-test"; 

    std::string serverAddress("testServeurDialogue");
    //const char *nomServ = std::string("testServeurDialogue").c_str();
    std::string clientName("Maurice");
    
    //  Set up broker, fty example server actor and third party actor
    zactor_t *server = zactor_new (mlm_server, (void*)"Malamute");
    zstr_sendx (server, "BIND", endpoint, NULL);
    if (verbose)
        zstr_send (server, "VERBOSE");

    //instanciate a server
    std::vector<std::string> arguments;
    arguments.push_back("Georges");
    zactor_t *example_server = zactor_new (fty_exdialogue_server, (void*) &arguments);
    zstr_sendx (example_server, "BIND", endpoint, serverAddress.c_str(), NULL);

    //instanciate a client
    mlm_client_t *client = mlm_client_new ();
    rv = mlm_client_connect (client, endpoint, 5000, "clientDialogue");
    assert(rv == 0);
    
    std::string knowName("Stranger"); //current name (anonymous)

    zmsg_t *message = NULL;
    char *nomD = NULL, *phrase = NULL;
    std::string expected;

    {
        //send 'hello' msg to server
        message = zmsg_new();
        zmsg_addstr (message, clientName.c_str());
        zmsg_addstr (message, "Hello !");
        rv = mlm_client_sendto (client, serverAddress.c_str(), "example", NULL, 1000, &message);
        assert(rv == 0);
        //rcv response
        message = mlm_client_recv (client);
        assert(message);
        //check response
        nomD = zmsg_popstr (message);
        phrase = zmsg_popstr (message);
        assert(nomD && phrase);
        expected = "Hello " + clientName + ".";
        if (verbose)
            zsys_debug("%s say : %s", knowName.c_str(), phrase);
        assert(streq(phrase, expected.c_str()));

        //cleanup
        zstr_free (&nomD);
        zstr_free (&phrase);
        zmsg_destroy (&message);
    }

    {
        //send 'who are you?' msg to server
        message = zmsg_new();
        zmsg_addstr (message, clientName.c_str());
        zmsg_addstr (message, "Who are you ?");
        rv = mlm_client_sendto (client, serverAddress.c_str(), "example", NULL, 1000, &message);
        assert(rv == 0);
        //rcv response
        message = mlm_client_recv (client);
        assert(message);
        //check response
        nomD = zmsg_popstr (message);
        phrase = zmsg_popstr (message);
        assert(nomD && phrase);
        expected = "My name is " + std::string(nomD);
        if (verbose)
            zsys_debug("%s say : %s", knowName.c_str(), phrase);
        assert(streq(phrase, expected.c_str()));

        //save name
        knowName = nomD;

        //cleanup
        zstr_free (&nomD);
        zstr_free (&phrase);
        zmsg_destroy (&message);
    }

    {
        //send 'goodbye' msg to server
        message = zmsg_new();
        zmsg_addstr (message, clientName.c_str());
        zmsg_addstr (message, std::string("Good bye " + knowName + " !").c_str());
        rv = mlm_client_sendto (client, serverAddress.c_str(), "example", NULL, 1000, &message);
        assert(rv == 0);
        //rcv response
        message = mlm_client_recv (client);
        assert(message);
        //check response
        nomD = zmsg_popstr (message);
        phrase = zmsg_popstr (message);
        assert(nomD && phrase);
        expected = "Good bye " + clientName + ".";
        if (verbose)
            zsys_debug("%s say : %s", knowName.c_str(), phrase);
        assert(streq(phrase, expected.c_str()));

        //cleanup
        zstr_free (&nomD);
        zstr_free (&phrase);
        zmsg_destroy (&message); 
    }
    
    //cleanup
    mlm_client_destroy (&client);
    zactor_destroy (&example_server);
    zactor_destroy (&server);

    // @end
    zsys_info ("fty_exdialogue_server_test ended");
}
