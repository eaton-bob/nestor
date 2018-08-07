/*  =========================================================================
    fty_exdialogue_client - Actor Client

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
    fty_exdialogue_client - Actor Client
@discuss
@end
*/

#include <string>
#include <vector>

#include "fty_exdialogue_classes.h"

//handle mailboxes
static void
s_dialogue_mailbox(mlm_client_t *client, std::string clientName, char *servAddress)
{
    std::string knowName("Stranger");
    zmsg_t *message = NULL;
    int rv;

    //send 'hello !' welcome msg 
    {
        message = zmsg_new();
        assert(message);
        zmsg_addstr(message, clientName.c_str());
        zmsg_addstr(message, "Hello !");

        rv = mlm_client_sendto (client, servAddress, "example", NULL, 1000, &message);
        zmsg_destroy(&message);
 
        if (rv != 0) {
            zsys_error("CLIENT mlm_client_sendto() failed");
            return;
        }
    }
    // handle response
    {
        message = mlm_client_recv (client);
        if (!message) {
            zsys_error("CLIENT mlm_client_recv() failed");
            return;
        }

        char *inName = zmsg_popstr(message);
        char *sentence = zmsg_popstr(message);

        //check consistency
        int stop = 0;
        std::string expected = "Hello " + clientName + ".";
        if (!streq(sentence, expected.c_str()))
        {
           zsys_error("CLIENT Message expected: %s, received : %s", expected.c_str(), sentence);
           stop = 1; 
        } else {
            zsys_debug("CLIENT %s say : %s", knowName.c_str(), sentence);
        }

        //cleanup
        zstr_free(&inName);
        zstr_free(&sentence);
        zmsg_destroy(&message);

        if (stop)
            return;
    }

    // send 'who are you ?' msg
    {
        message = zmsg_new();
        assert(message);
        zmsg_addstr (message, clientName.c_str());
        zmsg_addstr (message, "Who are you ?");

        rv = mlm_client_sendto (client, servAddress, "example", NULL, 1000, &message);
        zmsg_destroy(&message);
        
        if (rv != 0) {
            zsys_error("CLIENT mlm_client_sendto() failed");
            return;
        }
    }
    //handle response
    {
        message = mlm_client_recv (client);
        if (!message) {
           zsys_error("CLIENT mlm_client_recv() failed");
           return;
        }

        char *inName = zmsg_popstr(message);
        char *sentence = zmsg_popstr(message);

        //check consistency
        int stop = 0;
        std::string expected = "My name is " + std::string(inName);
        if (!streq(sentence, expected.c_str()))
        {
           zsys_error("CLIENT Message expected: %s, received : %s", expected.c_str(), sentence);
           stop = 1;
        } else {
            zsys_debug("CLIENT %s say : %s", knowName.c_str(), sentence);

            //ok, save name
            knowName = inName;
        }

        //cleanup
        zstr_free(&inName);
        zstr_free(&sentence);
        zmsg_destroy(&message);

        if (stop)
            return;
    }

    //send 'bye' msg
    {
        message = zmsg_new();
        assert(message);
        zmsg_addstr (message, clientName.c_str());
        zmsg_addstr (message, std::string("Good bye " + knowName + " !").c_str());

        rv = mlm_client_sendto (client, servAddress, "example", NULL, 1000, &message);
        zmsg_destroy(&message);
        
        if (rv != 0) {
            zsys_error("CLIENT mlm_client_sendto() failed");
            return;
        }
    }
    //handle response
    {   
        message = mlm_client_recv (client);
        if (!message) {
           zsys_error("CLIENT mlm_client_recv() failed");
           return;
        }

        char *inName = zmsg_popstr(message);
        char *sentence = zmsg_popstr(message);
        
        //check consistency
        int stop = 0;
        std::string expected = "Good bye " + clientName + ".";
        if (!streq(sentence, expected.c_str()))
        {
           zsys_error("CLIENT Message expected: %s, received : %s", expected.c_str(), sentence);
           stop = 1;
        } else {
            zsys_debug("CLIENT %s say : %s", knowName.c_str(), sentence);
        }

        //cleanup
        zstr_free(&inName);
        zstr_free(&sentence);
        zmsg_destroy(&message);

        if (stop)
            return;
    }
    
    //ok
}

// client callback function
void
fty_exdialogue_client (zsock_t *pipe, void* args)
{
    int verbose = 1; //default
    
    std::vector<std::string> arguments = *(std::vector<std::string> *) args;
    std::string clientName = arguments[0];

    zsys_info("CLIENT %s starting...", clientName.c_str());

    //create malamute client
    mlm_client_t *client = mlm_client_new ();
    if (!client) {
        zsys_error("CLIENT mlm_client_new() failed.");
        return;
    }

    //create poller on main pipe
    zpoller_t *poller = zpoller_new (pipe, NULL);
    zsock_signal (pipe, 0); //SIG needed (wakeup)

    zsys_debug("CLIENT %s ready", clientName.c_str());
    
    //start polling
    const int TIMEOUT = 5000; //ms
    int stop = 0; //internal abort flag

    while (!zsys_interrupted && !stop) {
        //wait for inputs
        void *sock = zpoller_wait (poller, -1);

        if (sock == pipe) {
            zmsg_t *message = zmsg_recv (pipe);
            char *command = zmsg_popstr (message);
 
            assert(message);
            assert(command);
  
            //$TERM command implementation is required by zactor_t interface
            if (streq (command, "$TERM")) {
               zsys_debug("CLIENT %s %s", clientName.c_str(), command);
               stop = 1; //abort
            }//$TERM
 
            //BIND command
            if (streq (command, "BIND")) {      
                char *endpoint = zmsg_popstr(message);
                char *address = zmsg_popstr(message); //address
                assert(endpoint);
                assert(address);

                zsys_debug("CLIENT %s %s %s %s", clientName.c_str(), command, endpoint, address);

                if (!(endpoint && address))
                {
                    zsys_error ("CLIENT bad message content");
                    stop = 1; //abort                    
                } else {
                    int rv = mlm_client_connect (client, endpoint, TIMEOUT, address);
                    if (rv == -1) {
                        zsys_error (
                                "CLIENT mlm_client_connect() failed (endpoint = '%s',address = '%s')",
                                endpoint, address);
 
                        stop = 1; //abort                    
                    }
                }

                //cleanup
                zstr_free(&endpoint);
                zstr_free(&address);
            }//BIND

            //VERBOSE command
            if (streq (command, "VERBOSE")) {
                char *param = zmsg_popstr(message); //'true'/'false'
                assert(param);
                zsys_debug("SERVER %s %s %s", clientName.c_str(), command, param);
                verbose = streq(param, "true") ? 1 : 0;
                 //cleanup
                zstr_free(&param);                    
            }

            //START command
            if (streq (command, "START")) {
                char *serverAddress = zmsg_popstr(message);

                zsys_debug("CLIENT %s %s %s", clientName.c_str(), command, serverAddress);

                if (!serverAddress)
                {
                    zsys_error("CLIENT Incorrect message");
                    stop = 1; //abort
                } else {
                    //handle mailbox msg
                    s_dialogue_mailbox(client, clientName.c_str(), serverAddress);
                }
 
                //cleanup
                zstr_free(&serverAddress);
            }//START

            //cleanup
            zstr_free(&command);
            zmsg_destroy(&message);
            
            continue;
        }
        
        //TODO, test timeout
    }

    zsys_info("CLIENT %s ended", clientName.c_str());

    //cleanup
    mlm_client_destroy(&client);
    zpoller_destroy(&poller);
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
fty_exdialogue_client_test (bool verbose)
{
    // @selftest
    zsys_info ("fty_exdialogue_client_test started");
    // @end
    zsys_info ("fty_exdialogue_client_test ended");
}
