#include "mrbig.h"

/* disk

green Tue Jul 06 02:43:47 VS 2004 [ntserver]


Filesystem   1K-blocks     Used    Avail Capacity  Mounted
C              1249696   813056   436640    65%    /FIXED/C ()
E              1249888     4458  1245430     0%    /FIXED/E ()
*/

struct cfg {
	char *name;
	int yellow, red;
	struct cfg *next;
} *pcfg;

static void read_diskcfg(/*char *p*/)
{
	struct cfg *pc;
	char b[100], name[100];
	int y, r, n, i;

	pcfg = NULL;
	for (i = 0; get_cfg("disk", b, sizeof b, i); i++) {
		if (b[0] == '#') continue;
		n = sscanf(b, "%s %d %d", name, &y, &r);
		if (n != 3) continue;
		pc = big_malloc("read_diskcfg (node)", sizeof *pc);
		pc->name = big_strdup("read_diskcfg (name)", name);
		pc->yellow = y;
		pc->red = r;
		pc->next = pcfg;
		pcfg = pc;
	}
}

static void free_cfg(void)
{
	struct cfg *dl;

	while (pcfg) {
		dl = pcfg;
		pcfg = dl->next;
		free(dl->name);
		free(dl);
	}
}

static void lookup_limits(char *drive, int *yellow, int *red)
{
	struct cfg *pc;

	for (pc = pcfg; pc; pc = pc->next) {
		if (!strcasecmp(pc->name, drive)) {
			*yellow = pc->yellow;
			*red = pc->red;
			return;
		}
	}
	*yellow = dfyellow;
	*red = dfred;
}

void disk(char *b, int n)
{
	int i, j, yellow, red;
	char d[10], p[100], q[5000], r[5000], *color = "green";
	char cfgfile[1024], drive[10];
	DWORD drives = GetLogicalDrives();
	unsigned int dtype, pct;
	unsigned long long total_bytes, free_bytes;
	ULARGE_INTEGER tb, fb;

	snprintf(cfgfile, sizeof cfgfile, "%s%c%s", cfgdir, dirsep, "disk.cfg");
	read_cfg("disk", cfgfile);
	read_diskcfg(/*cfgfile*/);

	r[0] = '\0';
	snprintf(q, sizeof q, "%-10s %11s %11s %11s %8s %s\n",
		"Filesystem", "1k-blocks", "Used", "Avail", "Capacity",
		"Mounted");
	for (i = 'A'; i <= 'Z'; i++) {
		if (drives & 1) {
			snprintf(drive, sizeof drive, "%c", i);
			snprintf(d, sizeof d, "%c:\\", i);
			dtype = GetDriveType(d);
			if (dtype == DRIVE_FIXED) {
				if (!GetDiskFreeSpaceEx(d, NULL, &tb, &fb)) {
					total_bytes = free_bytes = 0;
					pct = 0;
				} else {
					total_bytes = tb.QuadPart;
					free_bytes = fb.QuadPart;
					pct = 100-100*free_bytes/total_bytes;
				}
				lookup_limits(drive, &yellow, &red);
				if (pct >= red) {
					snprcat(r, sizeof r,
						"&red %s (%d%%) has reached the PANIC level (%d%%)\n",
						d, pct, red);
					color = "red";
				} else if (!strcmp(color, "green")
					 && pct >= yellow) {
					snprcat(r, sizeof r,
						"&yellow %s (%d%%) has reached the WARNING level (%d%%)\n",
						d, pct, yellow);
					color = "yellow";
				}
				j = 0;
				/* Filesystem */
				snprintf(p, sizeof p,
					"%-10c %11lu %11lu %11lu %5d%%   /FIXED/%c\n",
					i,
					(unsigned long) (total_bytes / 1024),
					(unsigned long) ((total_bytes - free_bytes) / 1024),
					(unsigned long) (free_bytes / 1024),
					pct, i);
				strncat(q, p, sizeof q);
			}
		}
		drives >>= 1;
	}
	free_cfg();
	snprintf(b, n, "status %s.disk %s %s\n\n%s\n%s\n",
		mrmachine, color, now, r, q);
}