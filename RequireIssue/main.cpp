/*
Copyright (c) 2011, Andrew Gray
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of the <organization> nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <string>
using namespace std;


#ifndef NULL
#define NULL 0
#endif

const char* g_issueCommands[] =
{
	"close", "closed", "closes", "closing", "fix", "fixed", "fixes",
	"reopen", "reopens", "reopening",
	"addresses", "re", "references", "ref", "refs", "see",
	"", NULL
};

const char* g_issuePrefixes[] =
{
	"#", "issue ", "bug ", "ticket ",
	"", NULL
};

bool SplitEnv(char* env, string* envKey, string* envValue)
{
	if(env == NULL || envKey == NULL || envValue == NULL)
		return false;

	string sEnv = env;
	string::size_type splitIndex = sEnv.find('=');

	//If there's no '=', then the whole thing is the key and there's no value
	if(splitIndex == string::npos)
	{
		*envKey = sEnv;
		*envValue = "";
		return false;
	}

	//We have a '=', so the key is everything to the left of it
	*envKey = sEnv.substr(0, splitIndex);

	//If the '=' is the last character, then the value is empty
	if(splitIndex == sEnv.length()-1)
		*envValue = "";

	//Otherwise, the value is everything to the right of the '='
	else
		*envValue = sEnv.substr(splitIndex+1);

	return true;
}

bool ContainsIssue(string line)
{
	for(unsigned int i=0;i<line.length();i++)
	{
		char ch = line[i];
		if(ch >= 'A' && ch <= 'Z')
		{
			ch -= 'A';
			ch += 'a';
			line[i] = ch;
		}
	}

	for(int i=0;;i++)
	{
		string testCommand = g_issueCommands[i];
		if(testCommand.length() == 0)
			break;

		string::size_type testCommandIndex = line.find(testCommand);
		if(testCommandIndex == string::npos)
			continue;

		printf("Found potential command: '%s'\n", testCommand.c_str());

		for(int j=0;;j++)
		{
			string testPrefix = g_issuePrefixes[j];
			if(testPrefix.length() == 0)
				break;

			string::size_type testPrefixIndex = line.find(testPrefix, testCommandIndex);
			if(testPrefixIndex == string::npos)
				continue;

			printf("Found potential issue prefix: '%s'\n", testPrefix.c_str());

			string::size_type testIssueIndex = testPrefixIndex + testPrefix.length();
			string::size_type testIssueEndIndex = testIssueIndex;
			for(; testIssueEndIndex < line.length(); testIssueEndIndex++)
			{
				char ch = line[testIssueEndIndex];
				if(!isdigit(ch))
					break;
			}

			if(testIssueEndIndex == testIssueIndex)
				continue;

			string issueString = line.substr(testIssueIndex, (testIssueEndIndex - testIssueIndex) + 1);
			printf("Found issue number: '%s'\n", issueString.c_str());
			return true;
		}
	}

	printf("Line: %s\n", line.c_str());
	return false;
}

int main(int argc, char** argv, char** envv)
{
	map<string, string> environment;
	for(char** env = envv; *env != 0; env++)
	{
		string envKey, envValue;
		SplitEnv(*env, &envKey, &envValue);

		environment[envKey] = envValue;
	}

	map<string, string>::iterator nodeIter = environment.find(string("HG_NODE"));
	if(nodeIter == environment.end())
	{
		printf("Environment variable 'HG_NODE' not set\n");
		return 1;
	}

	string nodeId = nodeIter->second;
	if(nodeId.length() == 0)
	{
		printf("HG_NODE value is invalid\n");
		return 2;
	}

	char hgCommand[1024];
	sprintf_s(hgCommand, 1024, "hg log -r %s --template {desc} > ./.hg/RequireIssue.desc", nodeId.c_str());
	int hgCommandResult = system(hgCommand);
	if(hgCommandResult != 0)
	{
		printf("Failed to run hg log\n");
		return 3;
	}

	ifstream hgResultFile("./.hg/RequireIssue.desc");
	if(!hgResultFile.good())
	{
		printf("Failed to open RequireIssue.desc\n");
		return 4;
	}

	while(hgResultFile.good() && !hgResultFile.eof() && !hgResultFile.fail())
	{
		string line = "";
		getline(hgResultFile, line);
		if(ContainsIssue(line))
		{
			printf("\n---\nFound a valid issue.  All good.\n");
			return 0;
		}
	}

	hgResultFile.close();
	system("del ./.hg/RequireIssue.desc");

	printf("\n---\nNo issue reference found.\n");
	return 5;
}
