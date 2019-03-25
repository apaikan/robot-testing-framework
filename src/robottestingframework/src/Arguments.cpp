/*
 * Robot Testing Framework
 *
 * Copyright (C) 2015-2019 Istituto Italiano di Tecnologia (IIT)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#include <robottestingframework/Arguments.h>

#include <cstring>

#define C_MAXARGS 128 // max number of the command parametes

using namespace robottestingframework;

void Arguments::split(char* line, char** args)
{
    char* pTmp = strchr(line, ' ');

    if (pTmp) {
        *pTmp = '\0';
        pTmp++;
        while ((*pTmp) && (*pTmp == ' ')) {
            pTmp++;
        }
        if (*pTmp == '\0') {
            pTmp = nullptr;
        }
    }
    *args = pTmp;
}

void Arguments::parse(char* azParam,
                      int* argc,
                      char** argv)
{
    char* pNext = azParam;
    size_t i;
    int j;
    int quoted = 0;
    size_t len = strlen(azParam);

    // Protect spaces inside quotes, but lose the quotes
    for (i = 0; i < len; i++) {
        if ((!quoted) && ('"' == azParam[i])) {
            quoted = 1;
            azParam[i] = ' ';
        } else if ((quoted) && ('"' == azParam[i])) {
            quoted = 0;
            azParam[i] = ' ';
        } else if ((quoted) && (' ' == azParam[i])) {
            azParam[i] = '\1';
        }
    }

    // init
    memset(argv, 0x00, sizeof(char*) * C_MAXARGS);
    *argc = 1;
    argv[0] = azParam;

    while ((nullptr != pNext) && (*argc < C_MAXARGS)) {
        split(pNext, &(argv[*argc]));
        pNext = argv[*argc];

        if (nullptr != argv[*argc]) {
            *argc += 1;
        }
    }

    for (j = 0; j < *argc; j++) {
        len = strlen(argv[j]);
        for (i = 0; i < len; i++) {
            if ('\1' == argv[j][i]) {
                argv[j][i] = ' ';
            }
        }
    }
}
