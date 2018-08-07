/*  =========================================================================
    fty_exdialogue - Binary

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
    fty_exdialogue - Binary
@discuss
@end
*/

#include <vector>
#include <iostream>
#include <string>

//required classes (server, client)
#include "fty_exdialogue_classes.h"

// display help to console
static void console_help() {
    printf("fty-exdialogue [options]\n");
    printf("  --config / -c    configuration file\n");
    printf("  --verbose / -v   verbose test output\n");
    printf("  --help / -h      this information\n");
}

// app main entry
int main (int argc, char *argv [])
{
    bool verbose = false; //default
    char *config_file = NULL;
	zconfig_t *config = NULL;

	//parse arguments
    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        assert(arg);
        
        if (streq (arg, "--help") ||  streq (arg, "-h")) {
            console_help();
            return EXIT_FAILURE;
        } else if (streq (arg, "--verbose") || streq (arg, "-v")) {
            verbose = true;
        } else if (streq (arg, "--config") || streq (arg, "-c")) {
           char *param = (i < (argc - 1)) ? argv[i+1] : NULL;
 
           if (!param) {
               printf("ARG %s: missing config-file parameter\n", arg);
               return EXIT_FAILURE;
           }
           config_file = strdup(param);
           i++; //skip param arg
        } else {
            printf ("ARG %s: unknown\n", arg);
            return EXIT_FAILURE;
        }
    }//for

    //config vars (default)
    char *endpoint = (char*)"ipc://@/malamute";
    char *serverName = (char*)"Baptiste";
    char *client1Name = (char*)"Louis";
    char *client2Name = (char*)"Fabrice";

    //parse config file
    if (config_file) {
        zsys_debug ("CONFIG %s", config_file);
		
        config = zconfig_load(config_file);
        if (!config) {
            zsys_error("CONFIG Failed to load %s", config_file);
            free(config_file);
            return EXIT_FAILURE;
        }

        //verbose
        if (streq(zconfig_get (config, "server/verbose", "false"), "true")) {
            verbose = true;
        } else {
            verbose = false;
        }

        //endpoint, server and clients names
        endpoint = zconfig_get(config, "malamute/endpoint", endpoint);
        serverName = zconfig_get(config, "dialogue/serverdname", serverName);
        client1Name = zconfig_get(config, "dialogue/clientdname1", client1Name);
        client2Name = zconfig_get(config, "dialogue/clientdname2", client2Name);

        //actor_name = s_get (config, "malamute/address", actor_name);
        //fty_info_command = s_get (config, "fty-info/command", fty_info_command);
        //zconfig_destroy (&config);
        
        //done
        free(config_file);
        config_file = NULL;
    }//if

    //display config vars
    if (verbose) {
        zsys_info("APP %s starting...", argv[0]);
        zsys_info("APP endpoint: %s, serverName: %s, client1Name: %s, client2Name: %s",
            endpoint, serverName, client1Name, client2Name);
    }

    //actors decl.
    zactor_t *serverActor = NULL;
    zactor_t *client1Actor = NULL;
    zactor_t *client2Actor = NULL;

    // temp args vector
    std::vector<std::string> arguments;
 
    //create server actor and bind it
    arguments.clear();
    arguments.push_back(std::string(serverName));
    std::string serverAddress("servDialogue"+std::string(serverName));

    serverActor = zactor_new (fty_exdialogue_server, (void*)&arguments);
    assert(serverActor);
    zstr_sendx (serverActor, "BIND", endpoint, serverAddress.c_str(), NULL);
    
    //create client1 actor and bind it
    arguments.clear();
    arguments.push_back(std::string(client1Name));
    std::string client1Address("clientDia" + std::string(client1Name));

    client1Actor = zactor_new (fty_exdialogue_client, (void*)&arguments);
    assert(client1Actor);
    zstr_sendx(client1Actor, "BIND", endpoint, client1Address.c_str(), NULL);
    
    //create client2 actor and bind it
    arguments.clear();
    arguments.push_back(std::string(client2Name));
    std::string client2Address("clientDia" + std::string(client2Name));

    client2Actor = zactor_new (fty_exdialogue_client, (void*)&arguments);
    assert(client2Actor);
    zstr_sendx(client2Actor, "BIND", endpoint, client2Address.c_str(), NULL);

    //forward verbosity to actors
    if (false) { // ZZZ not really handled, skipped
        zstr_sendx(serverActor,  "VERBOSE", (verbose ? "true" : "false"), NULL);
        zstr_sendx(client1Actor, "VERBOSE", (verbose ? "true" : "false"), NULL);
        zstr_sendx(client2Actor, "VERBOSE", (verbose ? "true" : "false"), NULL);
    }
   
    //send 'start' message to clients/ each second   
    while (!zsys_interrupted) {
        zstr_sendx(client1Actor, "START", serverAddress.c_str(), NULL);
        zstr_sendx(client2Actor, "START", serverAddress.c_str(), NULL);
        sleep(1); //seconds 
    }

    //cleanup 
    zconfig_destroy(&config);
    //WARN: the last zactor_destroy() call generates a "Invalid argument (src/mutex.hpp:142)" message.
    //      When changing the order calls, it's always the latest call that gen. the message
    //      see https://github.com/zeromq/libzmq/issues/2991
	//		decl. issue #4
    zactor_destroy(&serverActor);
    zactor_destroy(&client1Actor);
//WA issue#4 : don't call latest zactor_destroy()
#ifndef WORKAROUND_ISSUE4
    zactor_destroy(&client2Actor);
#endif
	
    return EXIT_SUCCESS; //ok
}
