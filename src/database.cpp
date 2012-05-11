
#include "shared.h"
#include "entity.h"
#include "utility.h"
#include "account.h"
#include "database.h"
#include "globals.h"


static ifstream flogin;

Entity* DB_FindEntity(unsigned id)
{
	Entity* ent = new Entity;
	ifstream ifent(ENTITY_FILENAME, ios::in | ios::binary);

	if (ifent.fail())
		return NULL;

	
	ent->Read(ifent);
	while (!ifent.eof())
	{
		if (ent->id == id)
		{
			ifent.close();
			return ent;
		}
		ent->Read(ifent);
	}

	return NULL;
}


Entity* DB_FindEntity(Login *login)
{
	Login cur_login;

	if (!DB_Logins_Begin())
		ShutdownGame();

	if (!DB_Logins_Next(cur_login))
	{
		DB_Logins_End();
		return NULL;
	}

	do
	{
		if (cur_login == (*login))
		{
			DB_Logins_End();
			return DB_FindEntity(cur_login.id);
		}

	} while ( DB_Logins_Next(cur_login) );

	DB_Logins_End();

	return NULL;
}



bool DB_Logins_Begin()
{
	Login hclogin;

	
	flogin.clear();
	flogin.open(LOGIN_FILENAME, ios::in | ios::binary);

	if (flogin.fail())	// try to rebuild database using hardcoded data
	{
		flogin.close();

		cout << "Primary read of logins failed. Trying to rebuild...";

		ofstream oflogin(LOGIN_FILENAME, ios::out | ios::binary);
		if (oflogin.fail())
		{
			cout << "failed." << endl;;
			return false;
		}
		
		strcat(hclogin.user, "god");
		strcat(hclogin.password, "god");
		hclogin.id = ID_GOD;
		hclogin.Write(oflogin);
		oflogin.close();

		cout << "done." << endl;

		// try once more
		flogin.clear();
		flogin.open(LOGIN_FILENAME, ios::in | ios::binary);
		if (flogin.fail())
		{
			flogin.close();
			cout << "error: unable to read logins." << endl;
			return false;
		}
	}


	return true;
}



bool DB_Logins_Next(Login& login)
{
	if (flogin.eof())
		return false;

	flogin.read(login.user, MAX_USER);
	flogin.read(login.password, MAX_PASS);
	flogin.read((char*)&login.id, sizeof(int));

	return true;	
}



bool DB_Logins_End()
{
	flogin.close();
	return true;
}


bool DB_Logins_Write (Login& login)
{
	ofstream oflogin(LOGIN_FILENAME, ios::out | ios::binary | ios::app);
	if (oflogin.fail())
		return false;

	login.Write(oflogin);
	oflogin.close();

	return true;
}


bool DB_Entities_UpdateAll()
{
	Entity cur_ent;
	Entity *ent;
	ENT_IT entIt;
	char* tmpfile = "entities.tmp";
	char buffer[256];

	ofstream ofentity(tmpfile, ios::out | ios::binary);
	if (ofentity.fail())
	{
		cout << "error: failed to write entities." << endl;
		return false;
	}


	ifstream ifentity(ENTITY_FILENAME, ios::in | ios::binary);
	if (!ifentity.fail())
	{
	
		// First we load an entity. We then check to see if the entity is currently
		// active (globals). If it is, we write it. If it isn't active, the buffered
		// entity list is then checked. If found, then it is written and removed from ghosts.
		// Finally, if the entity isn't active or buffered, we simple rewrite the old data.
	
		cur_ent.Read(ifentity);

		while (!ifentity.eof())
		{
			if ( (ent = find_object(cur_ent.id, globals)) )
			{
				ent->Write(ofentity);
			}
			else if ( (ent = find_object(cur_ent.id, ghosts)) )
			{
				ent->Write(ofentity);
				entIt = findit(ent, ghosts);
				delete (*entIt);
				entIt = ghosts.erase(entIt);
				entIt--;
			}
			else
				cur_ent.Write(ofentity);

			cur_ent.Read(ifentity);
		}
	}

	// cycle through ghosts and write the rest down
	for (entIt=ghosts.begin(); entIt != ghosts.end(); entIt++)
	{
		ent = (*entIt);

		if (!(ent->temp_flags & ET_DESTROY))
			ent->Write(ofentity);

		entIt = ghosts.erase(entIt);
		delete ent;
		entIt--;
	}

	// cycle through any active entities that weren't written
	
	for (entIt=globals.begin(); entIt != globals.end(); entIt++)
	{
		ent = (*entIt);

		if ( !(ent->temp_flags & ET_WROTE) )
			ent->Write(ofentity);
		
		ent->temp_flags &= ~ET_WROTE;
	}

	ofentity.close();
	ofentity.clear();
	ifentity.close();
	ifentity.clear();

	ofentity.open(ENTITY_FILENAME, ios::out | ios::binary);
	if (ofentity.fail())
	{
		cout << "error: failed to write entities." << endl;
		return false;
	}

	ifentity.open(tmpfile, ios::in | ios::binary);
	if (ifentity.fail())
	{
		cout << "error: failed to readback from entities." << endl;
		return false;
	}

	// create temporary file
	while (!ifentity.eof())
	{
		ifentity.read(buffer, sizeof(buffer));
		ofentity.write(buffer, ifentity.gcount());
	}

	ofentity.close();
	ifentity.close();


	return true;
}



