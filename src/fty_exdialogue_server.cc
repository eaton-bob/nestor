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

static void
s_handle_mailbox (mlm_client_t *client, zmsg_t **message_p, std::string nom)
{
    assert (client);
    assert (message_p);
    if (*message_p == NULL)
        return;
    zmsg_t *message = *message_p;

    char *nomClient = zmsg_popstr(message);
    if (!nomClient ) {
        zmsg_destroy(&message);
        zmsg_t *reply  = zmsg_new ();
        zmsg_addstr (reply, "ERROR");

        int rv = mlm_client_sendto (client, mlm_client_sender (client), "example", NULL, 1000, &reply);
        if (rv != 0) {
            zmsg_destroy (&reply);
            zsys_error (
                    "mlm_client_sendto (sender = '%s', subject = '%s', timeout = '1000') failed.",
                    mlm_client_sender (client), "example");
        }
        return;
    }
    
    char *part = zmsg_popstr (message);
    if (!part) {
        zmsg_destroy (&message);
        zstr_free (&nomClient);        
        zmsg_t *reply  = zmsg_new ();
        zmsg_addstr (reply, "ERROR");

        int rv = mlm_client_sendto (client, mlm_client_sender (client), "example", NULL, 1000, &reply);
        if (rv != 0) {
            zmsg_destroy (&reply);
            zsys_error (
                    "mlm_client_sendto (sender = '%s', subject = '%s', timeout = '1000') failed.",
                    mlm_client_sender (client), "example");
        }
        return;
    }
    std::string reponse;
    zsys_debug("%s dit : %s", nomClient, part);
    
    if (streq (part, "Bonjour !")) {
        zmsg_t *reply  = zmsg_new ();
        reponse = "Bonjour " + std::string(nomClient) + ".";
        zmsg_addstr (reply, "Inconnu");
        zmsg_addstr (reply, reponse.c_str());

        int rv = mlm_client_sendto (client, mlm_client_sender (client), "example", NULL, 1000, &reply);
        if (rv != 0) {
            zmsg_destroy (&reply);
            zsys_error (
                    "mlm_client_sendto (sender = '%s', subject = '%s', timeout = '1000') failed.",
                    mlm_client_sender (client), "example");
        } 
    }else if(streq (part, "Qui es-tu ?")) {
        zmsg_t *reply = zmsg_new();
        reponse = "Je me nomme " + nom;
        zmsg_addstr (reply, nom.c_str());
        zmsg_addstr (reply, reponse.c_str());
                
        int rv = mlm_client_sendto(client, mlm_client_sender(client), "example", NULL, 1000, &reply);
        if(rv != 0){
            zmsg_destroy(&reply);
            zsys_error (
                    "mlm_client_sendto (sender = '%s', subject = '%s', timeout = '1000') failed.",
                    mlm_client_sender (client), "example");
        }
    } else if(streq (part, std::string("Au revoir" + nom + " !").c_str())) {
        zmsg_t *reply = zmsg_new();
        reponse = "Au revoir " + std::string(nomClient) + ".";
        zmsg_addstr(reply, nom.c_str());
        zmsg_addstr(reply, reponse.c_str());
        
        int rv = mlm_client_sendto(client, mlm_client_sender(client), "example", NULL, 1000, &reply);
        if(rv != 0){
            zmsg_destroy(&reply);
            zsys_error (
                    "mlm_client_sendto (sender = '%s', subject = '%s', timeout = '1000') failed.",
                    mlm_client_sender (client), "example");
        }
    }else{
        zmsg_t *reply  = zmsg_new ();
        zmsg_addstr (reply, "ERROR");
        //zmsg_addstr (reply, "Je ne comprend pas.");

        int rv = mlm_client_sendto (client, mlm_client_sender (client), "example", NULL, 1000, &reply);
        if (rv != 0) {
            zmsg_destroy (&reply);
            zsys_error (
                    "mlm_client_sendto (sender = '%s', subject = '%s', timeout = '1000') failed.",
                    mlm_client_sender (client), "example");
        }
    }
    zstr_free (&part);
    zstr_free (&nomClient);
    zmsg_destroy (&message);
}

void
fty_exdialogue_server (zsock_t *pipe, void* args)
{
    std::vector<std::string> arguments = *(std::vector<std::string> *) args;
    //const char * endpoint = arguments[0].c_str();
    std::string nom = arguments[0];
    
    
    //zsys_debug ("endpoint: %s \n", endpoint);
    zsys_debug("Nom du serveur : %s", nom.c_str());
    

    mlm_client_t *client = mlm_client_new ();
    if (client == NULL) {
        zsys_error ("mlm_client_new () failed.");
        return;
    }

    //int rv = mlm_client_connect (client, endpoint, 1000, "fty-example");
    

    zpoller_t *poller = zpoller_new (pipe, mlm_client_msgpipe (client), NULL);
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
                char *endpoint = zmsg_popstr (message);
                char *myname = zmsg_popstr (message);
                assert (endpoint && myname);
                int rv = mlm_client_connect (client, endpoint, 5000, myname);
                if (rv != 0) {
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
                zstr_free (&endpoint);
                zstr_free (&myname);                    
            }
            zstr_free (&actor_command);
            zmsg_destroy (&message);
            continue;
        }
        else if(which ==  mlm_client_msgpipe (client)){
            zmsg_t *message = mlm_client_recv (client);
            if (message == NULL) {
                zsys_error ("interrupted");
                break;
            }
            if (streq (mlm_client_command (client), "MAILBOX DELIVER")) {
                s_handle_mailbox (client, &message, nom);
            }
            else {
                zsys_warning (
                        "Unsupported malamute pattern: '%s'. Message subject: '%s', sender: '%s'.",
                        mlm_client_command (client), mlm_client_subject (client), mlm_client_sender (client));
                zmsg_destroy (&message);
            }
        
        }

        
    }

    mlm_client_destroy (&client);
    zpoller_destroy (&poller);
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
fty_exdialogue_server_test (bool verbose)
{

    //  @selftest
    
    printf (" * fty_exdialogue_server: ");
    std::string nomServ("testServeurDialogue");
    //const char *nomServ = std::string("testServeurDialogue").c_str();
    static const char* endpoint = "inproc://fty-exdialogue-server-test"; 
    std::string nomClient("Maurice");
    
     //  Set up broker, fty example server actor and third party actor
    zactor_t *server = zactor_new (mlm_server, (void*)"Malamute");
    zstr_sendx (server, "BIND", endpoint, NULL);
    if (verbose)
        zstr_send (server, "VERBOSE");
    
    
    std::vector<std::string> arguments;
    arguments.push_back("Georges");
    zactor_t *example_server = zactor_new (fty_exdialogue_server, (void*) &arguments);
    zstr_sendx (example_server, "BIND", endpoint, nomServ.c_str(), NULL);
    
    mlm_client_t *client = mlm_client_new ();
    int rv = mlm_client_connect (client, endpoint, 5000, "clientDialogue");
    assert(rv == 0);
    
    zmsg_t *message = zmsg_new();
    std::string nomConnu("Inconnu");
    zmsg_addstr (message, nomClient.c_str());
    zmsg_addstr (message, "Bonjour !");
    rv = mlm_client_sendto (client, nomServ.c_str(), "example", NULL, 1000, &message);
   assert(rv == 0);
    message = mlm_client_recv (client);
    assert(message);
    
    char *nomD = zmsg_popstr (message);
    char *phrase = zmsg_popstr (message);
    std::string attendu = "Bonjour " + nomClient + ".";
    assert(streq(phrase, attendu.c_str()));
    
    zsys_debug("%s dit : %s", nomConnu.c_str(), phrase);
    
    zstr_free (&nomD);
    zstr_free (&phrase);
    zmsg_destroy (&message);
    
    
    
    message = zmsg_new();
    zmsg_addstr (message, nomClient.c_str());
    zmsg_addstr (message, "Qui es-tu ?");
    rv = mlm_client_sendto (client, nomServ.c_str(), "example", NULL, 1000, &message);
    
    assert(rv == 0);
    
    message = mlm_client_recv (client);
    assert(message);
    
    nomD = zmsg_popstr (message);
    phrase = zmsg_popstr (message);
    attendu = "Je me nomme " + std::string(nomD);
    assert(streq(phrase, attendu.c_str()));
    
    zsys_debug("%s dit : %s", nomConnu.c_str(), phrase);
    
    nomConnu = nomD;
    zstr_free (&nomD);
    zstr_free (&phrase);
    zmsg_destroy (&message);
    
    
    message = zmsg_new();
    zmsg_addstr (message, nomClient.c_str());
    zmsg_addstr (message, std::string("Au revoir" + nomConnu + " !").c_str());
    
    rv = mlm_client_sendto (client, nomServ.c_str(), "example", NULL, 1000, &message);
    assert(rv == 0);
    message = mlm_client_recv (client);
    assert(message);
    
    nomD = zmsg_popstr (message);
    phrase = zmsg_popstr (message);
    attendu = "Au revoir " + nomClient + ".";
    assert(streq(phrase, attendu.c_str()));
    
    zsys_debug("%s dit : %s", nomConnu.c_str(), phrase);
    
    zstr_free (&nomD);
    zstr_free (&phrase);
    zmsg_destroy (&message); 
    
    mlm_client_destroy (&client);
    zactor_destroy (&example_server);
    zactor_destroy (&server);

    //  @end
    printf ("OK\n");
}
