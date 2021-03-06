/***************************************************************************
*   Copyright (C) 2017 by Cyril Jacquet                                 *
*   cyril.jacquet@skribisto.eu                                        *
*                                                                         *
*  Filename: upgrader.cpp                                                   *
*  This file is part of Skribisto.                                    *
*                                                                         *
*  Skribisto is free software: you can redistribute it and/or modify  *
*  it under the terms of the GNU General Public License as published by   *
*  the Free Software Foundation, either version 3 of the License, or      *
*  (at your option) any later version.                                    *
*                                                                         *
*  Skribisto is distributed in the hope that it will be useful,       *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
*  GNU General Public License for more details.                           *
*                                                                         *
*  You should have received a copy of the GNU General Public License      *
*  along with Skribisto.  If not, see <http://www.gnu.org/licenses/>. *
***************************************************************************/
#include "plmupgrader.h"
#include "skrsqltools.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

PLMUpgrader::PLMUpgrader(QObject *parent) : QObject(parent)
{}

SKRResult PLMUpgrader::upgradeSQLite(QSqlDatabase sqlDb)
{
    SKRResult result("SKRSqlTools::upgradeSQLite");

    // find DB version :
    double dbVersion =  SKRSqlTools::getProjectDBVersion(&result, sqlDb);

    // ---------------------------------

    // from 1.0 to 1.1
    if (dbVersion == 1.0) {
        double newDbVersion = 1.1;

        sqlDb.transaction();

        QSqlQuery query(sqlDb);
        QString   queryStr =
                "CREATE TABLE tbl_project_dict (l_project_dict_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL ON CONFLICT ROLLBACK UNIQUE ON CONFLICT ROLLBACK, t_word TEXT UNIQUE ON CONFLICT REPLACE NOT NULL ON CONFLICT ROLLBACK);"
                ;
        query.prepare(queryStr);
        query.exec();

        if(query.lastError().isValid()){
            result = SKRResult(SKRResult::Critical, "PLMUpgrader::upgradeSQLite", "sql_error");
            result.addData("SQLError", query.lastError().text());
            result.addData("SQL string", queryStr);
        }

        IFOK(result) {
            sqlDb.commit();
        }

        IFKO(result) {
            result = SKRResult(SKRResult::Critical, "PLMUpgrader::upgradeSQLite", "cant_upgrade");
            result.addData("dbVersion", newDbVersion);
            result.addData("SQLError",  query.lastError().text());
        }

        IFOKDO(result, result = PLMUpgrader::setDbVersion(sqlDb, newDbVersion));

        IFOK(result) {
            dbVersion = newDbVersion;
        }
    }

    IFKO(result) {
        return result;
    }

    // ---------------------------------

    // from 1.1 to 1.2
    if (dbVersion == 1.1) {
        double newDbVersion = 1.2;

        QSqlQuery query(sqlDb);
        QString   queryStr =
                R""""(PRAGMA foreign_keys = 0;

                CREATE TABLE temp_table AS SELECT * FROM tbl_sheet_note;

                DROP TABLE tbl_sheet_note;

                CREATE TABLE tbl_sheet_note (
                l_sheet_note_id INTEGER PRIMARY KEY ON CONFLICT ROLLBACK AUTOINCREMENT
                UNIQUE ON CONFLICT ROLLBACK
                NOT NULL ON CONFLICT ROLLBACK,
                l_sheet_code    INTEGER REFERENCES tbl_sheet (l_sheet_id) ON DELETE CASCADE
                NOT NULL ON CONFLICT ROLLBACK,
                l_note_code     INTEGER REFERENCES tbl_note (l_note_id) ON DELETE CASCADE
                NOT NULL ON CONFLICT ROLLBACK,
                b_synopsis      BOOLEAN NOT NULL ON CONFLICT ROLLBACK
                DEFAULT (0)
                );

                INSERT INTO tbl_sheet_note (
                l_sheet_note_id,
                l_sheet_code,
                l_note_code,
                b_synopsis
                )
                SELECT l_sheet_note_id,
                l_sheet_code,
                l_note_code,
                b_synopsis
                FROM temp_table;

                DROP TABLE temp_table;

                PRAGMA foreign_keys = 1;
                )"""";


                result = SKRSqlTools::executeSQLString(queryStr, sqlDb);


        IFKO(result) {
            result = SKRResult(SKRResult::Critical, "PLMUpgrader::upgradeSQLite", "cant_upgrade");
            result.addData("dbVersion", newDbVersion);
        }

        IFOKDO(result, result = PLMUpgrader::setDbVersion(sqlDb, newDbVersion));

        IFOK(result) {
            dbVersion = newDbVersion;
        }
    }

    IFKO(result) {
        return result;
    }

    // ---------------------------------

    // from 1.2 to 1.3
    if (dbVersion == 1.2) {
        double newDbVersion = 1.3;

        QSqlQuery query(sqlDb);
        QString   queryStr =
                R""""(
                PRAGMA foreign_keys = 0;

                CREATE TABLE tbl_stat_history (
                l_stat_history_id  INTEGER  PRIMARY KEY ON CONFLICT ROLLBACK AUTOINCREMENT
                UNIQUE ON CONFLICT ROLLBACK
                NOT NULL ON CONFLICT ROLLBACK,
                dt_saved           DATETIME,
                l_sheet_char_count INTEGER,
                l_sheet_word_count INTEGER,
                l_note_char_count  INTEGER,
                l_note_word_count  INTEGER
                );

                INSERT INTO tbl_stat_history (
                dt_saved,
                l_sheet_char_count,
                l_sheet_word_count
                )
                SELECT dt_saved,
                l_char_count,
                l_word_count
                FROM tbl_history;

                DROP TABLE tbl_history;

                PRAGMA foreign_keys = 1;

                )"""";


                result = SKRSqlTools::executeSQLString(queryStr, sqlDb);


        IFKO(result) {
            result = SKRResult(SKRResult::Critical, "PLMUpgrader::upgradeSQLite", "cant_upgrade");
            result.addData("dbVersion", newDbVersion);
        }

        IFOKDO(result, result = PLMUpgrader::setDbVersion(sqlDb, newDbVersion));

        IFOK(result) {
            dbVersion = newDbVersion;
        }
    }

    IFKO(result) {
        return result;
    }
    // ---------------------------------

    // from 1.3 to 1.4
    if (dbVersion == 1.3) {
        double newDbVersion = 1.4;


        QSqlQuery query(sqlDb);
        QString   queryStr =
                R""""(PRAGMA foreign_keys = 0;

                CREATE TABLE sqlitestudio_temp_table AS SELECT *
                FROM tbl_tag;

                DROP TABLE tbl_tag;

                CREATE TABLE tbl_tag (
                l_tag_id     INTEGER  PRIMARY KEY ASC ON CONFLICT ROLLBACK AUTOINCREMENT
                UNIQUE ON CONFLICT ROLLBACK
                NOT NULL ON CONFLICT ROLLBACK,
                t_name       TEXT     NOT NULL
                UNIQUE ON CONFLICT ROLLBACK,
                t_color      TEXT,
                t_text_color TEXT,
                dt_created   DATETIME NOT NULL ON CONFLICT ROLLBACK
                DEFAULT (CURRENT_TIMESTAMP),
                dt_updated   DATETIME NOT NULL ON CONFLICT ROLLBACK
                DEFAULT (CURRENT_TIMESTAMP)
                );

                INSERT INTO tbl_tag (
                l_tag_id,
                t_name,
                t_color,
                dt_created,
                dt_updated
                )
                SELECT l_tag_id,
                t_name,
                t_color,
                dt_created,
                dt_updated
                FROM sqlitestudio_temp_table;

                DROP TABLE sqlitestudio_temp_table;

                PRAGMA foreign_keys = 1;

                )"""";


                result = SKRSqlTools::executeSQLString(queryStr, sqlDb);


        IFKO(result) {
            result = SKRResult(SKRResult::Critical, "PLMUpgrader::upgradeSQLite", "cant_upgrade");
            result.addData("dbVersion", newDbVersion);
        }

        IFOKDO(result, result = PLMUpgrader::setDbVersion(sqlDb, newDbVersion));

        IFOK(result) {
            dbVersion = newDbVersion;
        }

    }

    IFKO(result) {
        return result;
    }
    // ---------------------------------

    // from 1.4 to 1.5
    if (dbVersion == 1.4) {
        double newDbVersion = 1.5;

        // fill here
    }
    return result;
}

SKRResult PLMUpgrader::setDbVersion(QSqlDatabase sqlDb, double newVersion) {
    SKRResult result("SKRSqlTools::setDbVersion");

    sqlDb.transaction();
    QSqlQuery query(sqlDb);
    QString   queryStr = "UPDATE tbl_project SET dbl_database_version = :newVersion;";

    query.prepare(queryStr);
    query.bindValue(":newVersion", newVersion);
    query.exec();

    if(query.lastError().isValid()){
        result = SKRResult(SKRResult::Critical, "PLMUpgrader::setDbVersion", "sql_error");
        result.addData("SQLError", query.lastError().text());
        result.addData("SQL string", queryStr);
    }

    // update date
    IFOK(result) {
        queryStr = "UPDATE tbl_project SET dt_updated = CURRENT_TIMESTAMP;";
        query.prepare(queryStr);
        query.exec();

        if(query.lastError().isValid()){
            result = SKRResult(SKRResult::Critical, "PLMUpgrader::setDbVersion", "sql_error");
            result.addData("SQLError", query.lastError().text());
            result.addData("SQL string", queryStr);
        }
    }


    sqlDb.commit();

    return result;
}
