/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set sw=4 ts=4 et: */
/* ***** BEGIN LICENSE BLOCK *****
 *  Version: GPL 3
 * 
 *  This program is Copyright (C) 2007-2008 Aptana, Inc. All Rights Reserved
 *  This program is licensed under the GNU General Public license, version 3 (GPL).
 * 
 *  This program is distributed in the hope that it will be useful, but
 *  AS-IS and WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, TITLE, or
 *  NONINFRINGEMENT. Redistribution, except as permitted by the GPL,
 *  is prohibited.
 * 
 *  You can redistribute and/or modify this program under the terms of the GPL, 
 *  as published by the Free Software Foundation.  You should
 *  have received a copy of the GNU General Public License, Version 3 along
 *  with this program; if not, write to the Free Software Foundation, Inc., 51
 *  Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *  
 *  Aptana provides a special exception to allow redistribution of this file
 *  with certain other code and certain additional terms
 *  pursuant to Section 7 of the GPL. You may view the exception and these
 *  terms on the web at http://www.aptana.com/legal/gpl/.
 *  
 *  You may view the GPL, and Aptana's exception and additional terms in the file
 *  titled license-jaxer.html in the main distribution folder of this program.
 *  
 *  Any modifications to this file must keep this entire header intact.
 * 
 * ***** END LICENSE BLOCK ***** */
#ifndef FCGID_PROTOCOL_H
#define FCGID_PROTOCOL_H

#include "httpd.h"
#include "../../../../mozilla/aptana/jaxerProtocolVersion.h"


/*
 * Max bucket size for breaking brigade buckes
 */
#define  MAX_PACKET_SIZE			(8192)

/*
 * Jaxer protocol header definition
 */
typedef struct {
	unsigned char type;
	unsigned char contentLengthB1;
	unsigned char contentLengthB0;
} Jaxer_Header;

/*
 * Size of the Jaxer protocol header
 */
#define JAXER_HEADER_LEN            (3)

/*
 * Header type
 */
enum eHeaderType
{
    eREQUEST_HEADER = 0,
    eRESPONSE_HEADER = 1,
    eENVIRON_HDR = 2
};

#endif
