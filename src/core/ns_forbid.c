/* NickServ core functions
 *
 * (C) 2003-2009 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 *
 * $Id$
 *
 */
/*************************************************************************/

#include "module.h"

class CommandNSForbid : public Command
{
 public:
	CommandNSForbid() : Command("FORBID", 1, 2, "nickserv/forbid")
	{
	}

	CommandReturn Execute(User *u, std::vector<ci::string> &params)
	{
		NickAlias *na;
		const char *nick = params[0].c_str();
		const char *reason = params.size() > 1 ? params[1].c_str() : NULL;

		/* Assumes that permission checking has already been done. */
		if (ForceForbidReason && !reason) {
			this->OnSyntaxError(u);
			return MOD_CONT;
		}

		if (readonly)
			notice_lang(s_NickServ, u, READ_ONLY_MODE);
		if (!ircdproto->IsNickValid(nick))
		{
			notice_lang(s_NickServ, u, NICK_X_FORBIDDEN, nick);
			return MOD_CONT;
		}
		if ((na = findnick(nick)))
		{
			if (NSSecureAdmins && na->nc->IsServicesOper())
			{
				notice_lang(s_NickServ, u, ACCESS_DENIED);
				return MOD_CONT;
			}
			delete na;
		}
		na = new NickAlias(nick, new NickCore(nick));
		if (na)
		{
			na->SetFlag(NS_FORBIDDEN);
			na->last_usermask = sstrdup(u->nick);
			if (reason)
				na->last_realname = sstrdup(reason);

			User *curr = finduser(na->nick);

			if (curr)
			{
				notice_lang(s_NickServ, curr, FORCENICKCHANGE_NOW);
				collide(na, 0);
			}


			if (ircd->sqline)
				ircdproto->SendSQLine(na->nick, reason ? reason : "Forbidden");

			if (WallForbid)
				ircdproto->SendGlobops(s_NickServ, "\2%s\2 used FORBID on \2%s\2", u->nick, nick);

			alog("%s: %s set FORBID for nick %s", s_NickServ, u->nick, nick);
			notice_lang(s_NickServ, u, NICK_FORBID_SUCCEEDED, nick);

			FOREACH_MOD(I_OnNickForbidden, OnNickForbidden(na));
		}
		else
		{
			alog("%s: Valid FORBID for %s by %s failed", s_NickServ, nick, u->nick);
			notice_lang(s_NickServ, u, NICK_FORBID_FAILED, nick);
		}
		return MOD_CONT;
	}

	bool OnHelp(User *u, const ci::string &subcommand)
	{
		notice_help(s_NickServ, u, NICK_SERVADMIN_HELP_FORBID);
		return true;
	}

	void OnSyntaxError(User *u)
	{
		syntax_error(s_NickServ, u, "FORBID", ForceForbidReason ? NICK_FORBID_SYNTAX_REASON : NICK_FORBID_SYNTAX);
	}
};

class NSForbid : public Module
{
 public:
	NSForbid(const std::string &modname, const std::string &creator) : Module(modname, creator)
	{
		this->SetAuthor("Anope");
		this->SetVersion("$Id$");
		this->SetType(CORE);

		this->AddCommand(NICKSERV, new CommandNSForbid());

		ModuleManager::Attach(I_OnNickServHelp, this);
	}
	void OnNickServHelp(User *u)
	{
		notice_lang(s_NickServ, u, NICK_HELP_CMD_FORBID);
	}
};

MODULE_INIT(NSForbid)
