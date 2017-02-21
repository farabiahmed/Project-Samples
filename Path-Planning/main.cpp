//
//  main.cpp
//  Path Planning Algorithms
//
//  Created by Majid Moghadam on 6/22/16.
//  Copyright © 2016 UUBF. All rights reserved.
//

#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "M_LOS.hpp"
#include "VisibilityGraph.hpp"
#include "M_edges_mat.hpp"
#include "M_Vertex_Value.hpp"
#include "Coordinate.hpp"
#include "Polygon.hpp"

using namespace std;

//Compilation
//g++ main.cpp VisibilityGraph.cpp -o PathFinder

Coordinate PointA;		//Initial Point Of Path
Coordinate PointB;		//End Point of Path

vector<Polygon> NoFlyZones;
Polygon NoFlyZones_Push;
Coordinate Points_Push;

int NumberOfNoFlyZones=0;

int AlghoritmId=0;		//Defines which algorithm will be used.

//Algorithm Instances
VisibilityGraph VGraph;

//Coordinate* Path;
vector<Coordinate> Path;

//Example usage:
//pathfinder 1 41.1004679994753 29.0244787931442 40.9983990659513 29.2174530029297
int main(int argc, char* argv[])
{
    cout << "...Shortest Path Finding Routine..." << endl;
    
    // Check the number of parameters
    if (argc < 2) {
        
        //Tell the user how to run the program correctly.
        cerr << "[!] Usage: " << argv[0] << "<AlgortihmId> <LatA> <LonA> <LatB> <LonB>" << endl;
        cerr << "[!] Example: " << argv[0] << "1 41.1004679994753 29.0244787931442 40.9983990659513 29.2174530029297" << endl;
        cerr << "[!] Program terminating..." << endl;
        
        return 1;
    }
    
    //Program Configuration
    cout.precision(12);
    
    AlghoritmId = atoi(argv[1]);
    
    //Convert Arguments to variables
    PointA.Lat = atof(argv[2]);
    PointA.Lon = atof(argv[3]);
    PointB.Lat = atof(argv[4]);
    PointB.Lon = atof(argv[5]);
    
    //Print Out Variables
    cout <<"Latitude  A: " << PointA.Lat << endl;
    cout <<"Longitude A: " << PointA.Lon << endl;
    cout <<"Latitude  B: " << PointB.Lat << endl;
    cout <<"Longitude B: " << PointB.Lon << endl;
    
    
    //NoFlyZone Polygones
    ifstream file("NoFlyZone.nfz2");
    string line;
    
    while (getline(file, line))
    {
        NumberOfNoFlyZones++;
        
        istringstream ss(line);
        string token;
        
        //Get Altitude
        getline(ss, token, ',');
        NoFlyZones_Push.Altitude = atof(token.c_str());
        
        NoFlyZones_Push.EdgeCount=0;
        while(getline(ss, token, ','))
        {
            NoFlyZones_Push.EdgeCount++;
            
            //Get Latitude
            Points_Push.Lat = atof(token.c_str());
            
            //Get Longitude
            getline(ss, token, ',');
            Points_Push.Lon = atof(token.c_str());
            
            NoFlyZones_Push.Points.push_back(Points_Push);
        }
         NoFlyZones.push_back(NoFlyZones_Push);
         NoFlyZones_Push.Points.erase(NoFlyZones_Push.Points.begin(), NoFlyZones_Push.Points.end());
    }
    
    file.close();
    
    //Inform User about NoFly Zones
    cout<<"Number Of No Fly Zones: " << NumberOfNoFlyZones<<endl;

	  double maxAltitude=0;
    for(int i=0;i<NoFlyZones.size();i++)
    {
        cout<<"No Fly Zone #"<<i<<endl;
        cout<<"Altitude:" << NoFlyZones[i].Altitude << endl;
		
		if(NoFlyZones[i].Altitude > maxAltitude)
			maxAltitude = NoFlyZones[i].Altitude;
        
        for(int j=0;j<NoFlyZones[i].Points.size();j++)
        {
            cout<<"Coord #" << j << ": (" << NoFlyZones[i].Points[j].Lat << ","  << NoFlyZones[i].Points[j].Lon << ")" << endl;
        }
    }	
    
    //Run Algorithm
    switch(AlghoritmId)
    {
        case 1:
            //VisibilityGraph
            cout<<"VisibilityGraph Running..."<<endl;
            Path = VGraph.Run(&PointA, &PointB,NoFlyZones);
            break;
            
        case 2:
            //Astar
            cout<<"Astar Running..."<<endl;
            //Not implemented yet
            break;
            
        case 3:
            //RRT
            cout<<"RRT Running..."<<endl;
            //Not implemented yet
            break;
            
        case 4:
            //RRTStar
            cout<<"RRTStar Running..."<<endl;
            //Not implemented yet
            break;
            
        default:
            
            break;
    }
    
	
    
    //List the Coordinates Of Calculated Path
    cout<<"Generated Path:"<<endl;
    
    //Open a file in write mode to write Path
   	ofstream outfile;
    outfile.precision(12);
   	outfile.open("GeneratedPath.dat");
    outfile<<"Success,";
    outfile<<Path.size()<<",";
   	//Write Initial Point
   	//outfile<<PointA.Lat<<","<<PointA.Lon<<",";
   	//Write Each Path Node
   	for(int i=0;i<Path.size();i++)
    {
		Path[i].Alti = (int)maxAltitude;
        outfile<<Path[i].Lat<<","<<Path[i].Lon<<","<<Path[i].Alti<<",";
        
        cout<<Path[i].Lat<<","<<Path[i].Lon<<","<<Path[i].Alti<<endl;
    }
   	//Write End Point
   	//outfile<<PointB.Lat<<","<<PointB.Lon<<",";
    outfile<<"end";
   	//Close File
   	outfile.close();
    
    /*
    //Free Memory
    for(int i=0;i<NumberOfNoFlyZones;i++)
    {
        free(NoFlyZones[i].Points);
    }
    free(NoFlyZones);
     */
    NoFlyZones.erase(NoFlyZones.begin(), NoFlyZones.end());
    //free(Path);
    
    cout<<"End Of PathFinder..."<<endl;
    
    return 0;
}
