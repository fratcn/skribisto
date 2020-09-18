/***************************************************************************
 *   Copyright (C) 2020 by Cyril Jacquet                                 *
 *   cyril.jacquet@skribisto.eu                                        *
 *                                                                         *
 *  Filename: SKRTools.h                                                   *
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
#ifndef SKRQMLTOOLS_H
#define SKRQMLTOOLS_H


#include <QFileInfo>
#include <QObject>
#include <QUrl>


class SKRQMLTools : public QObject
{
    Q_OBJECT
public:
    explicit SKRQMLTools(QObject *parent = nullptr): QObject(parent){

    }


    Q_INVOKABLE QString translateURLToLocalFile(const QUrl &url) const{
        return url.toLocalFile();

    }
Q_INVOKABLE QUrl getFolderPathURLFromURL(const QUrl &url) const{

        QFileInfo info(url.toLocalFile());
        QString folderPath = info.path();
        return QUrl(folderPath);

    }



};

#endif // SKRQMLTOOLS_H
