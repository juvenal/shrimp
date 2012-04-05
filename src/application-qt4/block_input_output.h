
/*
    Copyright 2012, Romain Behar <romainbehar@users.sourceforge.net>

    This file is part of Shrimp 2.

    Shrimp 2 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Shrimp 2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Shrimp 2.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef BLOCK_INPUT_OUTPUT_H
#define BLOCK_INPUT_OUTPUT_H

#include "src/services.h"

#include <QDialog>

namespace Ui {
    class blockInputOutputDialog;
}


class block_input_output: public QDialog
{
    Q_OBJECT

public:
    explicit block_input_output (QWidget *parent, services* shrimpServicesInstance);
    ~block_input_output();

public slots:
    void cancelButton();
    void okButton();

private:
    Ui::blockInputOutputDialog *ui;
    services* shrimpServices;
};
#endif // BLOCK_INPUT_OUTPUT_H
