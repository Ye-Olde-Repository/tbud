
#ifndef _DATABASE_H_
#define _DATABASE_H_


/*  NEW DATABASE OUTLINE


  
  
  // Stores ent's info in database. Searches for prior existance (by id)
  // and updates that. If it doesn't exist, it creates a new row for ent.
  // returns -1 on failure
  int DB_WriteEntity(const Entity* ent);
  
  // Same as DB_WriteEntity, but for ClientInfo and associated table.
  int DB_WriteClientInfo(const ClientInfo* cli);

  int DB_Write


*/



Entity* DB_FindEntity(Login* login);

// Logins
bool DB_Logins_Begin();
bool DB_Logins_Next(Login& login);
bool DB_Logins_End();
bool DB_Logins_Write (Login& login);
bool DB_Entities_UpdateAll();
Entity* DB_FindEntity(unsigned id);

#endif

