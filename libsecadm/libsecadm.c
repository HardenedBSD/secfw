/*-
 * Copyright (c) 2014,2015 Shawn Webb <shawn.webb@hardenedbsd.org>
 * Copyright (c) 2015 Brian Salcedo <brian.salcedo@hardenedbsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <stdlib.h>
#include <malloc_np.h>
#include <sys/mount.h>

#include "secadm.h"

int
_secadm_sysctl(secadm_command_t *cmd, secadm_reply_t *reply)
{
	size_t cmdsz, replysz;
	int err;

	cmdsz = sizeof(secadm_command_t);
	replysz = sizeof(secadm_reply_t);

	err = sysctlbyname("hardening.secadm.control", reply, &replysz,
			   cmd, cmdsz);

	if (err) {
		perror("sysctlbyname");
		return (err);
	}

	if (reply->sr_code != secadm_reply_success) {
		fprintf(stderr, "control channel returned error code %d\n", reply->sr_code);
		return (reply->sr_code);
	}

	return (0);
}

int
secadm_flush_ruleset(void)
{
	int err;

	SECADM_SYSCTL_ARGS(secadm_cmd_flush_ruleset);

	if ((err = _secadm_sysctl(&cmd, &reply)))
		fprintf(stderr, "could not flush rules. error code: %d\n", err);

	return (err);
}

int
_secadm_rule_ops(secadm_rule_t *rule, secadm_command_type_t cmd_type)
{
	int err;

	SECADM_SYSCTL_ARGS(cmd_type);

	cmd.sc_data = rule;

	if ((err = _secadm_sysctl(&cmd, &reply)))
		fprintf(stderr, "secadm_rule_ops. error code: %d\n", err);

	return (err);
}

int
secadm_load_ruleset(secadm_rule_t *ruleset)
{
	return (_secadm_rule_ops(ruleset, secadm_cmd_load_ruleset));
}

int
secadm_add_rule(secadm_rule_t *rule)
{
	return (_secadm_rule_ops(rule, secadm_cmd_add_rule));
}

int
secadm_del_rule(int rule_id)
{
	secadm_rule_t rule;

	memset(&rule, 0, sizeof(secadm_rule_t));
	rule.sr_id = rule_id;

	return (_secadm_rule_ops(&rule, secadm_cmd_del_rule));
}

int
secadm_enable_rule(int rule_id)
{
	secadm_rule_t rule;

	memset(&rule, 0, sizeof(secadm_rule_t));
	rule.sr_id = rule_id;

	return (_secadm_rule_ops(&rule, secadm_cmd_enable_rule));
}

int
secadm_disable_rule(int rule_id)
{
	secadm_rule_t rule;

	memset(&rule, 0, sizeof(secadm_rule_t));
	rule.sr_id = rule_id;

	return (_secadm_rule_ops(&rule, secadm_cmd_disable_rule));
}

void *
_secadm_get_rule_data(secadm_rule_t *rule, size_t size)
{
	void *rule_data;
	int err;

	SECADM_SYSCTL_ARGS(secadm_cmd_get_rule_data);

	if ((rule_data = malloc(size)) == NULL) {
		perror("malloc");
		return NULL;
	}

	cmd.sc_data = rule;
	reply.sr_data = rule_data;

	printf("_secadm_get_rule_data()\n");
	if ((err = _secadm_sysctl(&cmd, &reply))) {
		fprintf(stderr, "unable to get rule data. error code: %d\n", err);
		return NULL;
	}

	return (rule_data);
}

u_char *
_secadm_get_rule_path(secadm_rule_t *rule)
{
	u_char *rule_path;
	int err;

	SECADM_SYSCTL_ARGS(secadm_cmd_get_rule_path);

	if ((rule_path = malloc(MAXPATHLEN + 1)) == NULL) {
		perror("malloc");
		return NULL;
	}

	memset(rule_path, 0, MAXPATHLEN);

	cmd.sc_data = rule;
	reply.sr_data = rule_path;

	printf("_secadm_get_rule_path()\n");

	if ((err = _secadm_sysctl(&cmd, &reply))) {
		fprintf(stderr, "unable to get rule path. error code: %d\n", err);
		return NULL;
	}

	return (rule_path);
}

u_char *
_secadm_get_rule_hash(secadm_rule_t *rule)
{
	u_char *rule_hash;
	int err;

	SECADM_SYSCTL_ARGS(secadm_cmd_get_rule_hash);

	if ((rule_hash = malloc(SECADM_SHA256_DIGEST_LEN + 1)) == NULL) {
		perror("malloc");
		return NULL;
	}

	memset(rule_hash, 0, SECADM_SHA256_DIGEST_LEN);
	cmd.sc_data = rule;
	reply.sr_data = rule_hash;

	printf("_secadm_get_rule_hash()\n");

	if ((err = _secadm_sysctl(&cmd, &reply))) {
		fprintf(stderr, "unable to get rule hash. error code: %d\n", err);
		return NULL;
	}

	return (rule_hash);
}

secadm_rule_t *
secadm_get_rule(int rule_id)
{
	secadm_rule_t *rule;
	size_t size;
	int err;

	SECADM_SYSCTL_ARGS(secadm_cmd_get_rule);

	if ((rule = malloc(sizeof(secadm_rule_t))) == NULL) {
		perror("malloc");
		return (NULL);
	}

	memset(rule, 0, sizeof(secadm_rule_t));

	rule->sr_id = rule_id;
	cmd.sc_data = rule;
	reply.sr_data = rule;

	printf("secadm_get_rule()\n");

	if ((err = _secadm_sysctl(&cmd, &reply))) {
		fprintf(stderr, "unable to get rule. error code: %d\n", err);
		secadm_free_rule(rule);

		return (NULL);
	}

	printf("rule_id: %d\n", rule_id);
	printf("rule id: %d\n", rule->sr_id);

	switch (rule->sr_type) {
	case secadm_integriforce_rule:
		rule->sr_integriforce_data =
		    _secadm_get_rule_data(rule, sizeof(secadm_integriforce_data_t));
		rule->sr_integriforce_data->si_path = _secadm_get_rule_path(rule);
		rule->sr_integriforce_data->si_hash = _secadm_get_rule_hash(rule);

		break;
	case secadm_pax_rule:
		rule->sr_pax_data =
		    _secadm_get_rule_data(rule, sizeof(secadm_pax_data_t));
		rule->sr_pax_data->sp_path = _secadm_get_rule_path(rule);

		break;
	case secadm_extended_rule:
		rule->sr_extended_data = _secadm_get_rule_data(rule, sizeof(secadm_extended_data_t));

		if (rule->sr_extended_data->sm_object.mo_pathsz)
			rule->sr_extended_data->sm_object.mo_path =
			    _secadm_get_rule_path(rule);
	}

	return (rule);
}

int
secadm_get_num_rules(void)
{
	int err, num_rules = -1;
	secadm_command_t cmd;
	secadm_reply_t reply;

	memset(&cmd, 0, sizeof(secadm_command_t));
	memset(&reply, 0, sizeof(secadm_reply_t));

	cmd.sc_version = SECADM_VERSION;
	cmd.sc_type = secadm_cmd_get_num_rules;
	reply.sr_data = &num_rules;

	printf("cmd.sc_data = %p\n", cmd.sc_data);

	if ((err = _secadm_sysctl(&cmd, &reply))) {
		fprintf(stderr, "unable to get rules. error code: %d\n", err);
		return (-1);
	}

	printf("num_rules = %d\n", num_rules);
	return (num_rules);
}

void
secadm_free_rule(secadm_rule_t *rule)
{
	switch (rule->sr_type) {
	case secadm_integriforce_rule:
		if (rule->sr_integriforce_data)
			free(rule->sr_integriforce_data);

		break;

	case secadm_pax_rule:
		if (rule->sr_pax_data)
			free(rule->sr_pax_data);

		break;

	case secadm_extended_rule:
		if (rule->sr_extended_data)
			free(rule->sr_extended_data);

		break;
	}

	free(rule);
}
