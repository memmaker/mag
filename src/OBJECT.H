/*
 *  		MAG - A Dungeon Adventuring Game
 *
 *	Copyright (C) 1986,87,88 by Michael J. Teixeira
 *
 *  General permission to copy or modify, but not for profit, is hereby
 *  granted, provided that the above copyright notice is included.
 *
 */

struct permobj	  pobj[NUMOBJ] = {
						/* food 0-6 */

	"food ration",		(CEAT | CGROUP),
	"slime mold",		(CEAT | CGROUP),
	"beef jerky",		(CEAT | CGROUP),
	"moldy cheese wedge",	(CEAT | CGROUP),
	"stale bread roll",	(CEAT | CGROUP),
	"dried fruit",		(CEAT | CGROUP),
	"mushroom",		(CEAT | CGROUP),

						/* potions 7-26 */

	"blindness",		(SID | CDRINK | CTHROW | CGROUP),
	"confusion",		(CDRINK | CTHROW | CGROUP),
	"extra healing",	(CDRINK | CTHROW | CGROUP),
	"gain strength",	(CDRINK | CTHROW | CGROUP),
	"haste self",		(SID | CDRINK | CTHROW | CGROUP),
	"healing",		(CDRINK | CTHROW | CGROUP),
	"heroism",		(CDRINK | CTHROW | CGROUP),
	"invisibility", 	(SID | CDRINK | CTHROW | CGROUP),
	"magic detection",	(SID | CDRINK | CTHROW | CGROUP),
	"monster detection",	(SID | CDRINK | CTHROW | CGROUP),
	"paralysis",		(SID | CDRINK | CTHROW | CGROUP),
	"poison",		(SID | CDRINK | CTHROW | CGROUP),
	"raise level",		(SID | CDRINK | CTHROW | CGROUP),
	"restore strength",	(CDRINK | CTHROW | CGROUP),
	"see invisible",	(CDRINK | CTHROW | CGROUP),
	"self polymorph",	(SID | CDRINK | CTHROW | CGROUP),
	"thirst quenching",	(CDRINK | CTHROW | CGROUP),
	"delusion",		(CDRINK | CTHROW | CGROUP),
	"levitation",		(SID | CDRINK | CTHROW | CGROUP),
	"acid",			(CDRINK | CTHROW | CGROUP),

						/* scrolls 27-47 */

	"aggravate monster",	(CREAD | CGROUP),
	"negation",		(CREAD | CGROUP),
	"confuse monster",	(CREAD | CGROUP),
	"create monster",	(CREAD | CGROUP),
	"treasure finding",	(SID | CREAD | CGROUP),
	"enchant armor",	(SID | CREAD | CGROUP),
	"enchant weapon",	(SID | CREAD | CGROUP),
	"genocide",		(SID | CREAD | CGROUP),
	"hold monster", 	(CREAD | CGROUP),
	"identify",		(SID | CREAD | CGROUP),
	"continual light",	(SID | CREAD | CGROUP),
	"magic mapping",	(SID | CREAD | CGROUP),
	"remove curse", 	(CREAD | CGROUP),
	"restore charges",	(SID | CREAD | CGROUP),
	"scare monster",	(CREAD | CGROUP),
	"sleep",		(SID | CREAD | CGROUP),
	"teleportation",	(SID | CREAD | CGROUP),
	"object attraction",	(CREAD | CGROUP),
	"detect food",		(SID | CREAD | CGROUP),
	"galvanization",	(SID | CREAD | CGROUP),
	"the labyrinth",	(SID | CREAD | CGROUP),

						/* wands/staves/rods 48-68 */

	"drain life",		(SID | CZAP),		       /* rods */
	"illumination", 	(SID | CZAP),
	"wonder",		CZAP,
	"slow monster", 	CZAP,
	"polymorph",		(SID | CZAP),
	"monster invisibility",	CZAP,

	"digging",		(CZAP | CWIELD),		/* staffs */
	"curing",		(CZAP | CWIELD),
	"fear", 		(CZAP | CWIELD),
	"striking",		(CZAP | CWIELD),
	"fire", 		(SID | CZAP | CWIELD),
	"frost",		(SID | CZAP | CWIELD),
	"teleport away",	(CZAP | CWIELD),
	"teleport to",		(CZAP | CWIELD),

	"cancellation", 	CZAP,				/* wands */
	"lightning",		(SID | CZAP),
	"trap detection",	CZAP,
	"haste monster",	CZAP,
	"magic detection",	CZAP,
	"magic missile",	(SID | CZAP),
	"paralysis",		CZAP,

						/* rings 69-93 */

	"decrease damage",	(CHAND | CENCH),		/* cursed */
	"decrease hit points",	(SID | CHAND | CENCH),
	"vulnerability",	(SID | CHAND | CENCH),
	"weakness",		(SID | CHAND | CENCH),
	"fumbling",		CHAND,
	"adornment",		CHAND,
	"aggravate monster",	CHAND,
	"teleportation",	CHAND,

	"free action",		CHAND,				/* good */
	"sustain strength",	CHAND,
	"regeneration", 	CHAND,
	"searching",		CHAND,
	"see invisible",	CHAND,
	"stealth",		CHAND,
	"warmth",		CHAND,
	"fire resistance",	CHAND,
	"sustinance",		CHAND,
	"resurrection",		CHAND,
	"magic absorption",	CHAND,
	"randomness",		CHAND,
	"add strength", 	(SID | CHAND | CENCH),
	"dexterity",		(CHAND | CENCH),
	"increase damage",	(CHAND | CENCH),
	"increase hit points",	(SID | CHAND | CENCH),
	"protection",		(SID | CHAND | CENCH),

						/* weapons 94-108 */

	"arrow",		(CTHROW | CMWENCH | CENCH | CGROUP),
	"long bow",		(CWIELD | CENCH),
	"crossbow bolt",	(CTHROW | CMWENCH | CENCH | CGROUP),
	"crossbow",		(CWIELD | CENCH),
	"sling bullet", 	(CTHROW | CMWENCH | CENCH | CGROUP),
	"sling",		(CWIELD | CENCH),
	"dagger",		(CWIELD | CTHROW | CENCH),
	"dart", 		(CTHROW | CMWENCH | CENCH | CGROUP),
	"flail",		(CWIELD | CENCH),
	"hand axe",		(CWIELD | CTHROW | CENCH),
	"long sword",		(CWIELD | CENCH),
	"mace", 		(CWIELD | CENCH),
	"boulder",		(CTHROW | CMWENCH | CENCH),
	"spear",		(CWIELD | CTHROW | CENCH),
	"two-handed sword",	(CWIELD | CENCH),

						/* armor 109-118 */

	"banded",		(CWEAR | CENCH),
	"chain",		(CWEAR | CENCH),
	"coconut",		(CWEAR | CENCH),
	"leather",		(CWEAR | CENCH),
	"padded",		(CWEAR | CENCH),
	"plate",		(CWEAR | CENCH),
	"ring", 		(CWEAR | CENCH),
	"scale",		(CWEAR | CENCH),
	"splint",		(CWEAR | CENCH),
	"studded leather",	(CWEAR | CENCH),

						/* shields 119-122 */

	"buckler",		(CSTRAP | CENCH),
	"medium shield",	(CSTRAP | CENCH),
	"great shield", 	(CSTRAP | CENCH),
	"mithril shield",	(CSTRAP | CENCH),

						/* keys 123-126 */

	"copper",		(CUNLOCK | CGROUP),
	"silver",		(CUNLOCK | CGROUP),
	"golden",		(CUNLOCK | CGROUP),
	"skeleton",		(CUNLOCK | CGROUP),

						/* miscellany 127-134 */

	"The Sudbury Saphire",	CUSE,
	"compass of translocation", CUSE,
	"cross",		CUSE,
	"mirror",		CUSE,
	"oil flask",		(CTHROW | CLIGHT | CGROUP),
	"tinderbox",		CUSE,
	"vial of ethereal oil",	CUSE,
	"chime of opening",	CUSE,

						/* gems 135-146 */

	"sparkling",		CGROUP,
	"crimson",		CGROUP,
	"purple",		CGROUP,
	"turquoise",		CGROUP,
	"opaque",		CGROUP,
	"green",		CGROUP,
	"opalescent",		CGROUP,
	"firey",		CGROUP,
	"brilliant",		CGROUP,
	"amber",		CGROUP,
	"glowing",		CGROUP,
	"xanthick",		CGROUP,

						/* treasure 147-150 */

	"platinum",		CGROUP,
	"gold", 		CGROUP,
	"electrum",		CGROUP,
	"silver",		CGROUP,
};

char	*fpotions[] = {
	"ebony",
	"magenta",
	"limpid",
	"emerald",
	"vermilion",
	"swirly",
	"silvery",
	"gold",
	"amethyst",
	"irridescent",
	"ochre",
	"smokey",
	"glowing",
	"bubbly",
	"oily",
	"viscous",
	"effervescent",
	"cloudy",
	"fuming",
	"syrupy",
};

char	*fwands[] = {
	"oaken",
	"ebony",
	"runed",
	"elongated",
	"teak",
	"saffron",
	"steel",
	"oblate",
	"iron",
	"marble",
	"frayed",
	"maple",
	"crystaline",
	"tapered",
	"mahogany",
	"whitewood",
	"cedar",
	"driftwood",
	"bone",
	"papyrus",
	"mithril",
};

char	*frings[] = {
	"quartz",
	"ivory",
	"lapis lazuli",
	"ruby",
	"jade",
	"diamond",
	"agate",
	"gold",
	"aquamarine",
	"white gold",
	"opal",
	"saphire",
	"pearl",
	"onyx",
	"tiger eye",
	"granite",
	"crystal",
	"red gold",
	"charcoal",
	"brass",
	"wooden",
	"iron",
	"mithril",
	"bone",
	"leather",
};

char	*fscrolls[] = {
	"Semper Ubi Sub Ubi",
	"Navigatum Iubet Vicissim",
	"E Pluribus Unix",
	"Tudo Poir",
	"Tuum Matrem Dat Bonum Capitem",
	"Ede Fecem Et Mora",
	"Feces In Mea Ora",
	"Pafonius A Um",
	"Voppter Zoy",
	"Oazubu Shzlaut Jaeg",
	"Apdaz A Ujore Uteo",
	"Ucme Hulag Sphitz",
	"Vas Sonarse",
	"Draco Te Edeat",
	"Capi Tuum Capitem Ex Tuo Asino",
	"Tiro Lasto Cuio",
	"Verbum Sapienti Est Satis",
	"Tua Mater Est Canem",
	"Rara Avis Defeceat In Tuo Capite",
	"Tuum Subligaculum Decadat",
	"Mein Bustenhalter Uberflute",
};
