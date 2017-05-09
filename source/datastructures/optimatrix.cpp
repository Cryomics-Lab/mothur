//
//  optimatrix.cpp
//  Mothur
//
//  Created by Sarah Westcott on 4/20/16.
//  Copyright (c) 2016 Schloss Lab. All rights reserved.
//

#include "optimatrix.h"
#include "progress.hpp"
#include "counttable.h"

/***********************************************************************/

OptiMatrix::OptiMatrix(string d, string df, double c, bool s) : distFile(d), distFormat(df), cutoff(c), sim(s) {
    m = MothurOut::getInstance();
    maxIndex = 0;
    countfile = ""; namefile = "";
    
    if (distFormat == "phylip") { readPhylip(); }
    else { readColumn();  }
}
/***********************************************************************/
OptiMatrix::OptiMatrix(string d, string nc, string f, string df, double c, bool s) : distFile(d), distFormat(df), format(f), cutoff(c), sim(s) {
    m = MothurOut::getInstance();
    maxIndex = 0;
    
    if (format == "name") { namefile = nc; countfile = ""; }
    else if (format == "count") { countfile = nc; namefile = ""; }
    else { countfile = ""; namefile = ""; }
    
    if (distFormat == "phylip") { readPhylip(); }
    else { readColumn();  }
}
/***********************************************************************/
int OptiMatrix::readFile(string d, string nc, string f, string df, double c, bool s)  {
    distFile = d; format = f; cutoff = c; sim = s; distFormat = df;
    
    if (format == "name") { namefile = nc; countfile = ""; }
    else if (format == "count") { countfile = nc; namefile = ""; }
    else { countfile = ""; namefile = ""; }
    
    if (distFormat == "phylip") { readPhylip(); }
    else { readColumn();  }
    
    return 0;
}
/***********************************************************************/
ListVector* OptiMatrix::getListSingle() {
    try {
        ListVector* singlelist = NULL;
        
        if (singletons.size() == 0) { }
        else {
            singlelist = new ListVector();
            
            for (int i = 0; i < singletons.size(); i++) {
                string otu = singletons[i];
                singlelist->push_back(otu);
            }
        }
        
        return singlelist;
    }
    catch(exception& e) {
        m->errorOut(e, "OptiMatrix", "getListSingle");
        exit(1);
    }
}
/***********************************************************************/
long int OptiMatrix::print(ostream& out) {
    try {
        long int count = 0;
        for (int i = 0; i < closeness.size(); i++) {
            for(set<int>::iterator it = closeness[i].begin(); it != closeness[i].end(); it++){
                out << *it << '\t';
                count++;
            }
            out << endl;
        }
        out << endl;
        return count;
    }
    catch(exception& e) {
        m->errorOut(e, "OptiMatrix", "getName");
        exit(1);
    }
}
/***********************************************************************/
string OptiMatrix::getName(int index) {
    try {
        if (index > maxIndex) { m->mothurOut("[ERROR]: index is not valid.\n"); m->control_pressed = true; return ""; }
        string name = nameMap[index];
        return name;
    }
    catch(exception& e) {
        m->errorOut(e, "OptiMatrix", "getName");
        exit(1);
    }
}
/***********************************************************************/
bool OptiMatrix::isClose(int i, int toFind){
    try {
        if (i > toFind) { int temp = i; i = toFind; toFind = temp; }
        
        bool found = false;
        if (closeness[i].count(toFind) != 0) { found = true; }
        return found;
        
    }
    catch(exception& e) {
        m->errorOut(e, "OptiMatrix", "isClose");
        exit(1);
    }
}
/***********************************************************************/

string OptiMatrix::findDistFormat(string distFile){
    try {
        string fileFormat = "column";
        
        ifstream fileHandle;
        string numTest;
        
        m->openInputFile(distFile, fileHandle);
        fileHandle >> numTest;
        fileHandle.close();
        
        if (m->isContainingOnlyDigits(numTest)) { fileFormat = "phylip"; }
        
        return fileFormat;
    }
    catch(exception& e) {
        m->errorOut(e, "OptiMatrix", "findDistFormat");
        exit(1);
    }
}
/***********************************************************************/

int OptiMatrix::readPhylip(){
    try {
        nameMap.clear();
        float distance;
        int square, nseqs;
        string name;
        map<int, int> singletonIndexSwap;
        
        ifstream fileHandle;
        string numTest;
        
        m->openInputFile(distFile, fileHandle);
        fileHandle >> numTest >> name;
        nameMap.push_back(name);
        singletonIndexSwap[0] = 0;
        
        if (!m->isContainingOnlyDigits(numTest)) { m->mothurOut("[ERROR]: expected a number and got " + numTest + ", quitting."); m->mothurOutEndLine(); exit(1); }
        else { convert(numTest, nseqs); }
        
        //square test
        char d;
        while((d=fileHandle.get()) != EOF){
            if(isalnum(d)){ square = 1; fileHandle.putback(d); for(int i=0;i<nseqs;i++){ fileHandle >> distance;  } break; }
            if(d == '\n'){ square = 0; break; }
        }
        
        vector<bool> singleton; singleton.resize(nseqs, true);
        ///////////////////// Read to eliminate singletons ///////////////////////
        if(square == 0){
            
            for(int i=1;i<nseqs;i++){
                if (m->control_pressed) {  fileHandle.close();  return 0; }
                
                fileHandle >> name; nameMap.push_back(name); singletonIndexSwap[i] = i;
                
                for(int j=0;j<i;j++){
                    
                    fileHandle >> distance;
                    
                    if (distance == -1) { distance = 1000000; } else if (sim) { distance = 1.0 - distance;  }  //user has entered a sim matrix that we need to convert.
                    
                    if(distance < cutoff){
                        singleton[i] = false;
                        singleton[j] = false;
                    }
                }
            }
        }else{
            for(int i=1;i<nseqs;i++){
                if (m->control_pressed) {  fileHandle.close();   return 0; }
                
                fileHandle >> name; nameMap.push_back(name); singletonIndexSwap[i] = i;
                
                for(int j=0;j<nseqs;j++){
                    fileHandle >> distance;
                    
                    if (distance == -1) { distance = 1000000; } else if (sim) { distance = 1.0 - distance;  }  //user has entered a sim matrix that we need to convert.
                    
                    if(distance < cutoff && j < i){
                        singleton[i] = false;
                        singleton[j] = false;
                    }
                }
            }
        }
        fileHandle.close();
        //////////////////////////////////////////////////////////////////////////
       
        int nonSingletonCount = 0;
        for (int i = 0; i < singleton.size(); i++) {
            if (!singleton[i]) { //if you are not a singleton
                singletonIndexSwap[i] = nonSingletonCount;
                nonSingletonCount++;
            }else { singletons.push_back(nameMap[i]); }
        }
        singleton.clear();

        closeness.resize(nonSingletonCount);
        
        map<string, string> names;
        if (namefile != "") {
            m->readNames(namefile, names);
            for (int i = 0; i < singletons.size(); i++) {
                singletons[i] = names[singletons[i]];
            }
        }
        
        Progress* reading;
        ifstream in;
        
        m->openInputFile(distFile, in);
        in >> nseqs >> name;
        
        if (namefile != "") { name = names[name]; } //redundant names
        nameMap[singletonIndexSwap[0]] = name;

        int fivepercent = (int)(0.05 * nseqs);
        
        string line = "";
        if(square == 0){
            
            reading = new Progress("Reading matrix:     ", nseqs * (nseqs - 1) / 2);
            int index = 0;
            
            for(int i=1;i<nseqs;i++){
                
                if (m->control_pressed) {  in.close();  delete reading; return 0; }
                
                in >> name; m->gobble(in);
                
                if (namefile != "") { name = names[name]; } //redundant names
                nameMap[singletonIndexSwap[i]] = name;
                
                for(int j=0;j<i;j++){
                    
                    in >> distance; m->gobble(in);
                    
                    if (distance == -1) { distance = 1000000; } else if (sim) { distance = 1.0 - distance;  }  //user has entered a sim matrix that we need to convert.
                    
                    if(distance < cutoff){
                        int newB = singletonIndexSwap[j];
                        int newA = singletonIndexSwap[i];
                        
                        //sparese storage
                        if (newB > newA) { int temp = newB; newB = newA; newA = temp; }
                        closeness[newA].insert(newB);
                    }
                    index++; reading->update(index);
                }
                
                if (m->debug) {
                    if((i % fivepercent) == 0){
                        unsigned long long ramUsed = m->getRAMUsed(); unsigned long long total = m->getTotalRAM();
                        m->mothurOut("\nCurrent RAM usage: " + toString(ramUsed/(double)GIG) + " Gigabytes. Total Ram: " + toString(total/(double)GIG) + " Gigabytes.\n");
                    }
                }
            }
        }else{
            reading = new Progress("Reading matrix:     ", nseqs * nseqs);
            int index = nseqs;
            
            for(int i=0;i<nseqs;i++){ in >> distance;  } m->gobble(in);
            
            for(int i=1;i<nseqs;i++){
                if (m->control_pressed) {  in.close();  delete reading; return 0; }
                
                in >> name; m->gobble(in);
                
                if (namefile != "") { name = names[name]; } //redundant names
                nameMap[singletonIndexSwap[i]] = name;
                
                for(int j=0;j<nseqs;j++){
                    in >> distance; m->gobble(in);

                    if (distance == -1) { distance = 1000000; } else if (sim) { distance = 1.0 - distance;  }  //user has entered a sim matrix that we need to convert.
                    
                    if(distance < cutoff && j < i){
                        int newB = singletonIndexSwap[j];
                        int newA = singletonIndexSwap[i];
                        //sparese storage
                        if (newB > newA) { int temp = newB; newB = newA; newA = temp; }
                        closeness[newA].insert(newB);
                    }
                    index++; reading->update(index);
                }
                
                if (m->debug) {
                    if((i % fivepercent) == 0){
                        unsigned long long ramUsed = m->getRAMUsed(); unsigned long long total = m->getTotalRAM();
                        m->mothurOut("\nCurrent RAM usage: " + toString(ramUsed/(double)GIG) + " Gigabytes. Total Ram: " + toString(total/(double)GIG) + " Gigabytes.\n");
                    }
                }
            }
        }
        in.close();
        reading->finish();
        delete reading;

        if (m->debug) { unsigned long long ramUsed = m->getRAMUsed(); unsigned long long total = m->getTotalRAM();
            m->mothurOut("\nCurrent RAM usage: " + toString(ramUsed/(double)GIG) + " Gigabytes. Total Ram: " + toString(total/(double)GIG) + " Gigabytes.\n"); }

        return 0;
        
    }
    catch(exception& e) {
        m->errorOut(e, "OptiMatrix", "readPhylip");
        exit(1);
    }
}
/***********************************************************************/

int OptiMatrix::readColumn(){
    try {
        map<string, int> nameAssignment;
        if (namefile != "") { nameAssignment = m->readNames(namefile); }
        else  {  CountTable ct; ct.readTable(countfile, false, true); nameAssignment = ct.getNameMap(); }
        int count = 0;
        for (map<string, int>::iterator it = nameAssignment.begin(); it!= nameAssignment.end(); it++) {
            it->second = count; count++;
            nameMap.push_back(it->first);
        }
        
        string firstName, secondName;
        float distance;
        
        ///////////////////// Read to eliminate singletons ///////////////////////
        ifstream fileHandle;
        m->openInputFile(distFile, fileHandle);
        
        map<int, int> singletonIndexSwap;
        vector<bool> singleton; singleton.resize(nameAssignment.size(), true);
        vector<bool> needsIndex; needsIndex.resize(nameAssignment.size(), false);
        while(fileHandle){  //let's assume it's a triangular matrix...
            
            fileHandle >> firstName; m->gobble(fileHandle);
            fileHandle >> secondName; m->gobble(fileHandle);
            fileHandle >> distance;	m->gobble(fileHandle); // get the row and column names and distance
            
            if (m->debug) { cout << firstName << '\t' << secondName << '\t' << distance << endl; }
            
            if (m->control_pressed) {  fileHandle.close();   return 0; }
            
            map<string,int>::iterator itA = nameAssignment.find(firstName);
            map<string,int>::iterator itB = nameAssignment.find(secondName);
            
            if(itA == nameAssignment.end()){  m->mothurOut("AAError: Sequence '" + firstName + "' was not found in the name or count file, please correct\n"); exit(1);  }
            if(itB == nameAssignment.end()){  m->mothurOut("ABError: Sequence '" + secondName + "' was not found in the name or count file, please correct\n"); exit(1);  }
            
            if (distance == -1) { distance = 1000000; }
            else if (sim) { distance = 1.0 - distance;  }  //user has entered a sim matrix that we need to convert.
            
            if(distance < cutoff){
                int indexA = (itA->second);
                int indexB = (itB->second);
                singleton[indexA] = false;
                singleton[indexB] = false;
                singletonIndexSwap[indexA] = indexA;
                singletonIndexSwap[indexB] = indexB;
                if (indexA > indexB) {  needsIndex[indexB] = true; }
                else { needsIndex[indexA] = true; }
            }
        }
        fileHandle.close();
        //////////////////////////////////////////////////////////////////////////
        
        int nonSingletonCount = 0;
        for (int i = 0; i < singleton.size(); i++) {
            if (!singleton[i]) { //if you are a singleton
                if (needsIndex[i]) {
                    singletonIndexSwap[i] = nonSingletonCount;
                    nonSingletonCount++;
                }
            }else { singletons.push_back(nameMap[i]); }
        }
        singleton.clear();
        
        ifstream in;
        m->openInputFile(distFile, in);
        
        closeness.resize(nonSingletonCount);
        
        map<string, string> names;
        if (namefile != "") {
            m->readNames(namefile, names);
            for (int i = 0; i < singletons.size(); i++) {
                singletons[i] = names[singletons[i]];
            }
        }
        cout << "here\n";
        while(in){  //let's assume it's a triangular matrix...
            
            in >> firstName; m->gobble(in);
            in >> secondName; m->gobble(in);
            in >> distance;	m->gobble(in); // get the row and column names and distance
            
            if (m->debug) { cout << firstName << '\t' << secondName << '\t' << distance << endl; }
            
            if (m->control_pressed) {  in.close();   return 0; }

            map<string,int>::iterator itA = nameAssignment.find(firstName);
            map<string,int>::iterator itB = nameAssignment.find(secondName);
            
            if(itA == nameAssignment.end()){  m->mothurOut("AAError: Sequence '" + firstName + "' was not found in the name or count file, please correct\n"); exit(1);  }
            if(itB == nameAssignment.end()){  m->mothurOut("ABError: Sequence '" + secondName + "' was not found in the name or count file, please correct\n"); exit(1);  }
            
            if (distance == -1) { distance = 1000000; }
            else if (sim) { distance = 1.0 - distance;  }  //user has entered a sim matrix that we need to convert.
            
            if(distance < cutoff){
                int indexA = (itA->second);
                int indexB = (itB->second);
                
                
                int newB = singletonIndexSwap[indexB];
                int newA = singletonIndexSwap[indexA];
                
                //sparese storage
                if (newB > newA) { int temp = newB; newB = newA; newA = temp; }
                //only add if not already stored
                if (newA < closeness.size()) { closeness[newA].insert(newB); }
                
                if (namefile != "") {
                    firstName = names[firstName];  //redundant names
                    secondName = names[secondName]; //redundant names
                }
                
                nameMap[newA] = firstName;
                nameMap[newB] = secondName;
            }
        }
        in.close();
        nameAssignment.clear();
        
        if (m->debug) { unsigned long long ramUsed = m->getRAMUsed(); unsigned long long total = m->getTotalRAM();
            m->mothurOut("\nCurrent RAM usage: " + toString(ramUsed/(double)GIG) + " Gigabytes. Total Ram: " + toString(total/(double)GIG) + " Gigabytes.\n"); }
        
        unsigned long long ramUsed = m->getRAMUsed(); unsigned long long total = m->getTotalRAM();
        m->mothurOut("\nCurrent RAM usage: " + toString(ramUsed/(double)GIG) + " Gigabytes. Total Ram: " + toString(total/(double)GIG) + " Gigabytes.\n");
        
        return 1;
        
    }
    catch(exception& e) {
        m->errorOut(e, "OptiMatrix", "readColumn");
        exit(1);
    }
}
/***********************************************************************/
