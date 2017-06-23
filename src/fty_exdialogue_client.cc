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

//  Structure of our class
static int
s_dialogue_mailbox(mlm_client_t *client, std::string clientName, char* servAddress)
{
    zmsg_t *message = zmsg_new();
    std::string knowName("Stranger");
    zmsg_addstr (message, clientName.c_str());
    zmsg_addstr (message, "Hello !");
    int rv = mlm_client_sendto (client, servAddress, "example", NULL, 1000, &message);
    if(rv != 0)
        return -1;
    message = mlm_client_recv (client);
    if(!message)
        return -1;
    
    char *nomD = zmsg_popstr (message);
    char *sentence = zmsg_popstr (message);
    std::string expected = "Hello " + clientName + ".";
    if(!streq(sentence, expected.c_str()))
    {
       zsys_error("Message expected : %s \n message get : %s", expected.c_str(), sentence);
       return -1; 
    }
    
    zsys_debug("%s say : %s", knowName.c_str(), sentence);
    
    zstr_free (&nomD);
    zstr_free (&sentence);
    zmsg_destroy (&message);
    
    
    
    message = zmsg_new();
    zmsg_addstr (message, clientName.c_str());
    zmsg_addstr (message, "Who are you ?");
    rv = mlm_client_sendto (client, servAddress, "example", NULL, 1000, &message);
    if(rv != 0)
        return -1;
    message = mlm_client_recv (client);
    if(!message)
        return -1;
    
    nomD = zmsg_popstr (message);
    sentence = zmsg_popstr (message);
    expected = "My name is " + std::string(nomD);
    if(!streq(sentence, expected.c_str()))
    {
       zsys_error("Message expected : %s \n message get : %s", expected.c_str(), sentence);
       return -1; 
    }
    
    zsys_debug("%s say : %s", knowName.c_str(), sentence);
    
    knowName = nomD;
    zstr_free (&nomD);
    zstr_free (&sentence);
    zmsg_destroy (&message);
    
    
    message = zmsg_new();
    zmsg_addstr (message, clientName.c_str());
    zmsg_addstr (message, std::string("Good bye " + knowName + " !").c_str());
    
    rv = mlm_client_sendto (client, servAddress, "example", NULL, 1000, &message);
    if(rv != 0)
        return -1;
    message = mlm_client_recv (client);
    if(!message)
        return -1;
    
    nomD = zmsg_popstr (message);
    sentence = zmsg_popstr (message);
    expected = "Good bye " + clientName + ".";
    if(!streq(sentence, expected.c_str()))
    {
       zsys_error("Message expected : %s \n message get : %s", expected.c_str(), sentence);
       return -1; 
    }
    
    zsys_debug("%s say : %s", knowName.c_str(), sentence);
    
    zstr_free (&nomD);
    zstr_free (&sentence);
    zmsg_destroy (&message);
    return 0;
}


void
fty_exdialogue_client (zsock_t *pipe, void* args)
{
    std::vector<std::string> arguments = *(std::vector<std::string> *) args;
    //const char * endpoint = arguments[0].c_str();
    //const char * servName = arguments[1].c_str();
    std::string name = arguments[0];
    char *serverAddress = NULL;
    
    
    //zsys_debug ("endpoint: %s \n", endpoint);
    zsys_debug("Client name : %s", name.c_str());
    

    mlm_client_t *client = mlm_client_new ();
    if (client == NULL) {
        zsys_error ("mlm_client_new () failed.");
        return;
    }
    
    

    zpoller_t *poller = zpoller_new (pipe, NULL);
    zsock_signal (pipe, 0);
    zsys_debug ("actor ready");

    while (!zsys_interrupted) {
        void *which = zpoller_wait (poller, -1);
        if (which == pipe) {
            zmsg_t *message = zmsg_recv (pipe);
            char *actor_command = zmsg_popstr (message);
            //  $TERM actor command implementation is required by zactor_t interface
            if (streq (actor_command, "$TERM")) {
                zstr_free (&actor_command);
                zmsg_destroy (&message);
                break;
            } else if (streq (actor_command, "BIND")) {      
                //zsys_debug ("bind ask");
                char *endpoint = zmsg_popstr (message);
                char *myname = zmsg_popstr (message);
                if(endpoint == NULL || myname == NULL)
                {
                    zsys_error ("bad message content");
                    zstr_free (&actor_command);
                    zmsg_destroy (&message);                    
                    zpoller_destroy (&poller);
                    break;                    
                }
                //assert (endpoint && myname);
                int rv = mlm_client_connect (client, endpoint, 5000, myname);
                if (rv == -1) {
                    mlm_client_destroy (&client);
                    zsys_error (
                            "mlm_client_connect (endpoint = '%s', timeout = '%d', address = '%s') failed.",
                            endpoint, 5000, myname);
                    zstr_free (&endpoint);
                    zstr_free (&myname);  
                    zstr_free (&actor_command);
                    zmsg_destroy (&message);                    
                    zpoller_destroy (&poller);
                    break;
                }
               // zsys_debug ("bind ok");
                zstr_free(&endpoint);
                zstr_free(&myname);
            } else if (streq (actor_command, "START")) {
                //zsys_debug ("start ask");
                serverAddress = zmsg_popstr (message);
                if(serverAddress == NULL)
                {
                    zsys_error ("Incorrect START message");
                    mlm_client_destroy (&client);
                    zstr_free (&actor_command);
                    zmsg_destroy (&message);                    
                    zpoller_destroy (&poller);
                    break;
                }
                
                //zsys_debug ("start ready");
                s_dialogue_mailbox(client, name,serverAddress);
                //zsys_debug ("start ok");
                zstr_free(&serverAddress);
                
            }
            zstr_free (&actor_command);
            zmsg_destroy (&message);
            continue;
        }
        
        //TODO, test timeout
    }

    mlm_client_destroy (&client);
    zpoller_destroy (&poller);
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
fty_exdialogue_client_test (bool verbose)
{
    printf ("OK\n");
}
