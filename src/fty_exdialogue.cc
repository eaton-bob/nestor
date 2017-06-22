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

#include "fty_exdialogue_classes.h"

int main (int argc, char *argv [])
{
    bool verbose = false;
    char *config_file = NULL;
    int argn;
    
    for (argn = 1; argn < argc; argn++) {
        char *param = NULL;
        if (argn < argc - 1) param = argv [argn+1];
        if (streq (argv [argn], "--help")
        ||  streq (argv [argn], "-h")) {
            puts ("fty-exdialogue [options] ...");
            puts ("  --verbose / -v         verbose test output");
            puts ("  --help / -h            this information");
            return 0;
        }
        else
        if (streq (argv [argn], "--verbose")
        ||  streq (argv [argn], "-v"))
        {
            verbose = true;
        }
        else if (streq (argv [argn], "--config") || streq (argv [argn], "-c")) {
            if (param) config_file = param;
            ++argn;
        }
        else {
            printf ("Unknown option: %s\n", argv [argn]);
            return 1;
        }
    }
    //  Insert main code here
    if (verbose)
        zsys_info ("fty_exdialogue - Binary");
    
    char *endpoint = (char*)"ipc://@/malamute";
    char *serverName = (char*) "Baptiste";
    char *clientName1 = (char*) "Louis";
    char *clientName2 = (char*) "Fabrice";
    zconfig_t *config = NULL;
    //parse config file
    if(config_file){
        //zconfig_t *config = NULL;
        printf ("fty_exdialogue:LOAD: %s \n", config_file);
        config = zconfig_load (config_file);
        if (!config) {
            printf("Failed to load config file %s: \n", config_file);
            exit (EXIT_FAILURE);
        }
        // VERBOSE
        if (streq (zconfig_get (config, "server/verbose", "false"), "true")) {
            verbose = true;
        }
        endpoint = zconfig_get (config, "malamute/endpoint", endpoint);
        serverName = zconfig_get(config, "dialogue/serverdname", serverName);
        clientName1 = zconfig_get(config, "dialogue/clientdname1", clientName1);
        clientName2 = zconfig_get(config, "dialogue/clientdname2", clientName2);
       //actor_name = s_get (config, "malamute/address", actor_name);
       // fty_info_command = s_get (config, "fty-info/command", fty_info_command);
       // zconfig_destroy (&config);
    }
    
   //création du serveur nestor
    std::string nomServnestor("servDialogue"+std::string(serverName));
    std::vector<std::string> arguments;
    arguments.push_back(std::string(serverName));
    
    zactor_t *example_server = zactor_new (fty_exdialogue_server, (void *) &arguments);
    zstr_sendx (example_server, "BIND", endpoint, nomServnestor.c_str(), NULL);
    
    //création du client Louis
    arguments.clear();
    arguments.push_back(std::string(clientName1));
    std::string nomClient1("clientDia"+std::string(clientName1));
    
    zactor_t *client_louis = zactor_new (fty_exdialogue_client, (void *) &arguments);
    zstr_sendx(client_louis, "BIND", endpoint, nomClient1.c_str(), NULL);
    
    
    //création du client Fabrice
    arguments.clear();
    arguments.push_back(std::string(clientName2));
    std::string nomClient2("clientDia"+std::string(clientName2));
    
    zactor_t *client_fabrice = zactor_new (fty_exdialogue_client, (void *) &arguments);
    zstr_sendx(client_fabrice, "BIND", endpoint, nomClient2.c_str(), NULL);
    
    
    
    //Lancement des clients :
    
    zstr_sendx(client_louis, "START", nomServnestor.c_str(), NULL);
    zstr_sendx(client_fabrice, "START", nomServnestor.c_str(), NULL);
    
    //zpoller_t *poller = zpoller_new (example_server, NULL);
    
    while (!zsys_interrupted) {
        zstr_sendx(client_louis, "START", nomServnestor.c_str(), NULL);
        zstr_sendx(client_fabrice, "START", nomServnestor.c_str(), NULL);
        sleep(1); 
    }
   /*
    //  Accept and print any message back from server
    //  copy from src/malamute.c under MPL license
    while (true) {
        char *message = zstr_recv (example_server);
        if (message) {
            puts (message);
            free (message);
        }
        else {
            puts ("interrupted");
            break;
        }
    }*/
    if(config_file)
        zconfig_destroy (&config);
    zactor_destroy (&example_server);
    zactor_destroy (&client_louis);
    zactor_destroy (&client_fabrice);
    return EXIT_SUCCESS;
}
