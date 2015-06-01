/* Test program for elf_update function.
   Copyright (C) 2000, 2001, 2002, 2005 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2000.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <fcntl.h>
#include <libelf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libebl.h>
#include <elf-print.h>

int main (int argc, char *argv[] __attribute__ ((unused)))
{
	int fd;
	Elf *elf;
	GElf_Ehdr *ehdr;
	GElf_Ehdr ehdr_mem;

	Elf_Scn *isoft_scn;

	Elf_Scn *str_scn;
	GElf_Shdr *str_shdr;

	GElf_Shdr *shdr;
	GElf_Shdr shdr_mem;

	GElf_Shdr *isoft_shdr;
	GElf_Shdr isoft_shdr_mem;

	Elf_Data *data;
	struct Ebl_Strtab *shst;
	struct Ebl_Strent *isoftse;
	struct Ebl_Strent *shstrtabse;
	int i;

	if(argc != 2) {
		printf("usage: %s <ELF>\n", argv[0]);
		exit(1);
	}

	fd = open (argv[1], O_RDWR);
	if (fd == -1)
	{
		printf ("cannot open `%s': %s\n", argv[1], strerror (errno));
		exit (1);
	}

	elf_version (EV_CURRENT);

	//elf_fill (0x42);

	if ((elf = elf_begin (fd, ELF_C_RDWR, NULL)) == NULL) {
		printf ("cannot create ELF descriptor: %s\n", elf_errmsg (-1));
		exit (1);
	}

	/* Get an ELF header.  */
	if ((ehdr = gelf_getehdr (elf, &ehdr_mem)) == NULL) {
		printf ("cannot get the ELF header: %s\n", elf_errmsg (-1));
		exit (1);
	}
	print_header(elf);

	/* 先保存 shstrtab section */
	if ((str_scn = elf_getscn (elf, ehdr->e_shstrndx)) == NULL)
	{
		printf ("cannot get SHSTRTAB section: %s\n", elf_errmsg (-1));
		exit (1);
	}

	print_section(elf, str_scn);

	/* 获得shstr header */
	if ((str_shdr = gelf_getshdr (str_scn, &shdr_mem)) == NULL) {
		printf ("cannot get header for SHSTRTAB section: %s\n", elf_errmsg (-1));
		exit (1);
	}

	/* add isoft section */
	shst = ebl_strtabinit (false);

	isoft_scn = elf_newscn (elf);
	if (isoft_scn == NULL)
	{
		printf ("cannot create first section: %s\n", elf_errmsg (-1));
		exit (1);
	}

	isoft_shdr = gelf_getshdr (isoft_scn, &isoft_shdr_mem);
	if (isoft_shdr == NULL)
	{
		printf ("cannot get header for first section: %s\n", elf_errmsg (-1));
		exit (1);
	}

	isoftse = ebl_strtabadd (shst, ".isoft", 0);

	//isoft_shdr->sh_type = SHT_PROGBITS;
	isoft_shdr->sh_type = SHT_LOUSER;
	//isoft_shdr->sh_flags = SHF_ALLOC | SHF_EXECINSTR;
	isoft_shdr->sh_flags = SHF_MERGE | SHF_STRINGS;
	isoft_shdr->sh_addr = 0;
	isoft_shdr->sh_link = 0;
	isoft_shdr->sh_info = 0;
	isoft_shdr->sh_entsize = 1;

	data = elf_newdata (isoft_scn);
	if (data == NULL)
	{
		printf ("cannot create data first section: %s\n", elf_errmsg (-1));
		exit (1);
	}

	data->d_buf = "hello";
	data->d_type = ELF_T_BYTE;
	data->d_version = EV_CURRENT;
	data->d_size = 5;
	data->d_align = 16;

	//print_section(elf, isoft_scn);
	/////////////////////
	
	str_shdr->sh_type = SHT_STRTAB;
	str_shdr->sh_flags = 0;
	str_shdr->sh_addr = 0;
	str_shdr->sh_link = SHN_UNDEF;
	str_shdr->sh_info = SHN_UNDEF;
	str_shdr->sh_entsize = 1;

	/* We have to store the section index in the ELF header.  */
	Elf_Data *newdata;
	newdata = elf_newdata (str_scn);
	if (data == NULL)
	{
		printf ("cannot create data SHSTRTAB section: %s\n", elf_errmsg (-1));
		exit (1);
	}
	data = elf_getdata (str_scn, NULL);
	//printf(">>>%d\n", data->d_size);

	//printf("==============\nshow table: %s:%d %s()\n", __FILE__, __LINE__, __FUNCTION__);
	//print_section(elf, str_scn);
	/* No more sections, finalize the section header string table. */
	printf(">>>offset=%ld, %ld\n", data->d_off, data->d_size);
	printf(">>>offset=%ld\n", data->d_size + 2 + ebl_strtaboffset(isoftse));
	ebl_strtabfinalize (shst, newdata);
	printf(">>>offset=%ld\n", ebl_strtaboffset(isoftse));

	/* 更新 shstrtable 表头 */
	if(gelf_update_shdr(str_scn, str_shdr) < 0) {
		printf("Failed to perform dynamic update: %s.", elf_errmsg(-1));
		exit(1);
	}

	printf("==============\nshow table: %s:%d %s()\n", __FILE__, __LINE__, __FUNCTION__);
	print_section(elf, str_scn);

	/* 先更新一下isoft 表头*/
	if(gelf_update_shdr(isoft_scn, isoft_shdr) < 0) {
		printf("Failed to perform dynamic update: %s.", elf_errmsg(-1));
		exit(1);
	}
	printf("==============\nshow table: %s:%d %s()\n", __FILE__, __LINE__, __FUNCTION__);
	print_section(elf, isoft_scn);

	printf("==============\nshow table: %s:%d %s()\n", __FILE__, __LINE__, __FUNCTION__);
	print_section(elf, str_scn);

	isoft_shdr = gelf_getshdr (isoft_scn, &isoft_shdr_mem);
	isoft_shdr->sh_name = data->d_size + ebl_strtaboffset (isoftse);

	/* 再更新一下isoft */
	if(gelf_update_shdr(isoft_scn, isoft_shdr) < 0) {
		printf("Failed to perform dynamic update: %s.", elf_errmsg(-1));
		exit(1);
	}

	//elf_flagelf (elf, ELF_C_SET, ELF_F_LAYOUT);

	/* Write out the file.  */
	if (elf_update (elf, ELF_C_NULL) < 0)
	{
		printf ("failure in elf_update(WRITE): %s\n", elf_errmsg (-1));
		exit (1);
	}

	printf("==============\nshow table: %s:%d %s()\n", __FILE__, __LINE__, __FUNCTION__);

	//str_scn = elf_getscn (elf, ehdr->e_shstrndx);
	//str_shdr = gelf_getshdr (str_scn, &shdr_mem);

	/* 更新 shstrtable */
	//if(gelf_update_shdr(str_scn, str_shdr) < 0) {
	//	printf("Failed to perform dynamic update: %s.", elf_errmsg(-1));
	//	exit(1);
	//}
	//print_section(elf, str_scn);

	//str_shdr = gelf_getshdr (str_scn, &shdr_mem);
	//str_shdr->sh_name = ebl_strtaboffset (shstrtabse);

	//printf("sh_name=%s\n", ebl_string (isoftse));

	//if(gelf_update_shdr(str_scn, str_shdr) < 0) {
	//	printf("Failed to perform dynamic update: %s.", elf_errmsg(-1));
	//	exit(1);
	//}

	//ehdr->e_shstrndx = elf_ndxscn (str_scn);

	/* Write out the file.  */
	if (elf_update (elf, ELF_C_WRITE) < 0)
	{
		printf ("failure in elf_update(WRITE): %s\n", elf_errmsg (-1));
		exit (1);
	}
	print_header(elf);

	//str_shdr = gelf_getshdr (str_scn, &shdr_mem);
	//str_shdr->sh_name = ebl_strtaboffset (shstrtabse);

	//if(gelf_update_shdr(str_scn, str_shdr) < 0) {
	//	printf("Failed to perform dynamic update: %s.", elf_errmsg(-1));
	//	exit(1);
	//}

	/* We don't need the string table anymore.  */
	ebl_strtabfree (shst);

	/* And the data allocated in the .shstrtab section.  */
	free (newdata->d_buf);

	if (elf_end (elf) != 0)
	{
		printf ("failure in elf_end: %s\n", elf_errmsg (-1));
		exit (1);
	}

	return 0;
}
