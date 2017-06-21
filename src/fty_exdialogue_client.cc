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
s_dialogue_mailbox(mlm_client_t *client, std::string nomClient, char* nomServ)
{
    zmsg_t *message = zmsg_new();
    std::string nomConnu("Inconnu");
    zmsg_addstr (message, nomClient.c_str());
    zmsg_addstr (message, "Bonjour !");
    int rv = mlm_client_sendto (client, nomServ, "example", NULL, 1000, &message);
    if(rv != 0)
        return -1;
    message = mlm_client_recv (client);
    if(!message)
        return -1;
    
    char *nomD = zmsg_popstr (message);
    char *phrase = zmsg_popstr (message);
    std::string attendu = "Bonjour " + nomClient + ".";
    if(!streq(phrase, attendu.c_str()))
    {
       zsys_error("Message attendu : %s \n message obtenu : %s", attendu.c_str(), phrase);
       return -1; 
    }
    
    zsys_debug("%s dit : %s", nomConnu.c_str(), phrase);
    
    zstr_free (&nomD);
    zstr_free (&phrase);
    zmsg_destroy (&message);
    
    
    
    message = zmsg_new();
    zmsg_addstr (message, nomClient.c_str());
    zmsg_addstr (message, "Qui es-tu ?");
    rv = mlm_client_sendto (client, nomServ, "example", NULL, 1000, &message);
    if(rv != 0)
        return -1;
    message = mlm_client_recv (client);
    if(!message)
        return -1;
    
    nomD = zmsg_popstr (message);
    phrase = zmsg_popstr (message);
    attendu = "Je me nomme " + std::string(nomD);
    if(!streq(phrase, attendu.c_str()))
    {
       zsys_error("Message attendu : %s \n message obtenu : %s", attendu.c_str(), phrase);
       return -1; 
    }
    
    zsys_debug("%s dit : %s", nomConnu.c_str(), phrase);
    
    nomConnu = nomD;
    zstr_free (&nomD);
    zstr_free (&phrase);
    zmsg_destroy (&message);
    
    
    message = zmsg_new();
    zmsg_addstr (message, nomClient.c_str());
    zmsg_addstr (message, std::string("Au revoir" + nomConnu + " !").c_str());
    
    rv = mlm_client_sendto (client, nomServ, "example", NULL, 1000, &message);
    if(rv != 0)
        return -1;
    message = mlm_client_recv (client);
    if(!message)
        return -1;
    
    nomD = zmsg_popstr (message);
    phrase = zmsg_popstr (message);
    attendu = "Au revoir " + nomClient + ".";
    if(!streq(phrase, attendu.c_str()))
    {
       zsys_error("Message attendu : %s \n message obtenu : %s", attendu.c_str(), phrase);
       return -1; 
    }
    
    zsys_debug("%s dit : %s", nomConnu.c_str(), phrase);
    
    zstr_free (&nomD);
    zstr_free (&phrase);
    zmsg_destroy (&message);
    return 0;
}


void
fty_exdialogue_client (zsock_t *pipe, void* args)
{
    std::vector<std::string> arguments = *(std::vector<std::string> *) args;
    //const char * endpoint = arguments[0].c_str();
    //const char * servName = arguments[1].c_str();
    std::string nom = arguments[0];
    char *nomServ = NULL;
    
    
    //zsys_debug ("endpoint: %s \n", endpoint);
    zsys_debug("Nom du client : %s", nom.c_str());
    

    mlm_client_t *client = mlm_client_new ();
    if (client == NULL) {
        zsys_error ("mlm_client_new () failed.");
        return;
    }
    
    

    zpoller_t *poller = zpoller_new (pipe, NULL);
    zsock_signal (pipe, 0);
    zsys_debug ("actor ready");

    while (!zsys_interrupted) {
        void *which = zpoller_wait (poller, 1000);
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
                    return;                    
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
                    return;
                }
               // zsys_debug ("bind ok");
                zstr_free(&endpoint);
                zstr_free(&myname);
            } else if (streq (actor_command, "START")) {
                //zsys_debug ("start ask");
                nomServ = zmsg_popstr (message);
                if(nomServ == NULL)
                {
                    zsys_error ("Incorrect START message");
                    mlm_client_destroy (&client);
                    zstr_free (&actor_command);
                    zmsg_destroy (&message);                    
                    zpoller_destroy (&poller);
                    return;
                }
                
                //zsys_debug ("start ready");
                s_dialogue_mailbox(client, nom,nomServ);
                //zsys_debug ("start ok");
                zstr_free(&nomServ);
                
            }
            zstr_free (&actor_command);
            zmsg_destroy (&message);
            continue;
        }
        
        //TODO, tester timeout
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
