#include <asm-generic/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "femail.h"
#include "comm.h"

void init_smtp(smtpcontext * ctx) {
	ctx->active = false;
	ctx->starttls = false;
	ctx->socket4 = -1;
	ctx->socket6 = -1;
}

void init_smtps(smtpscontext * ctx) {
	ctx->active = false;
	ctx->socket4 = -1;
	ctx->socket6 = -1;
}

void init_starttls(starttlscontext * ctx) {
	ctx->active = false;
	ctx->socket4 = -1;
	ctx->socket6 = -1;
}

void init_http(httpcontext * ctx) {
	ctx->active = false;
	ctx->socket4 = -1;
	ctx->socket6 = -1;
}

void init_https(httpscontext * ctx) {
	ctx->active = false;
	ctx->socket4 = -1;
	ctx->socket6 = -1;
}


int start_smtp(smtpcontext * ctx) {
	if (ctx->active) {
		return 0;
	}

	if (sslctx == NULL) {
		log_warn("SMTP: No SSL context available. StartTLS disabled.");
		ctx->starttls = false;
	} else {
		ctx->starttls = true;
	}
	
	ctx->socket4 = socket(AF_INET,
						  SOCK_STREAM | SOCK_NONBLOCK,
						  0);
	if (ctx->socket4 == -1) {
		log_err("SMTP: Failed to create IPv4 socket.");
	} else {
		int opt = 1;
		if (setsockopt(ctx->socket4,
				   SOL_SOCKET,
				   SO_REUSEADDR,
				   &opt,
					   sizeof(opt)) != 0) {
			log_warn("SMTP: Couldn't set an IPv4 socket option.");
		}

		struct sockaddr_in addr4 = {0};
		addr4.sin_family = AF_INET;
		addr4.sin_addr.s_addr = INADDR_ANY;
		addr4.sin_port = htons(SMTP_PORT);

		if (bind(ctx->socket4,
				(struct sockaddr *)&addr4,
				sizeof(addr4)) != 0) {
			log_crit("SMTP: Unable to bind on the IPv4 socket. %s",
					 strerror(errno));
			close(ctx->socket4);
			ctx->socket4 = -1;
		} else {
			if (listen(ctx->socket4,
					   SOMAXCONN) != 0) {
				log_err("SMTP: Unable to listen on the IPv4 socket. %s",
						strerror(errno));
				close(ctx->socket4);
				ctx->socket4 = -1;
			} else {
				ctx->active = true;
			}
		}
	}

	ctx->socket6 = socket(AF_INET6,
						  SOCK_STREAM | SOCK_NONBLOCK,
						  0);
	if (ctx->socket6 == -1) {
		log_err("SMTP: Failed to create IPv6 socket.");
	} else {
		int opt = 1;
		if (setsockopt(ctx->socket6,
					   SOL_SOCKET,
					   SO_REUSEADDR,
					   &opt,
					   sizeof(opt)) != 0) {
			log_warn("SMTP: Couldn't set an IPv6 socket option.");
		}

		int v6opt = 1;
		if (setsockopt(ctx->socket6,
					   IPPROTO_IPV6,
					   IPV6_V6ONLY,
					   &v6opt,
					   sizeof(v6opt)) != 0) {
			log_warn("SMTP: Couldn't set an IPv6 socket option.");
		}

		struct sockaddr_in6 addr6 = {0};
		addr6.sin6_family = AF_INET6;
		addr6.sin6_addr = in6addr_any;
		addr6.sin6_port = htons(SMTP_PORT);

		if (bind(ctx->socket6,
				 (struct sockaddr *)&addr6,
				 sizeof(addr6)) != 0) {
			log_crit("SMTP: Unable to bind on the IPv6 socket. %s",
					 strerror(errno));
			close(ctx->socket6);
			ctx->socket6 = -1;
		} else {
			if (listen(ctx->socket6,
					   SOMAXCONN) != 0) {
				log_err("SMTP: Unable to listen on the IPv6 socket. %s",
						strerror(errno));
				close(ctx->socket6);
				ctx->socket6 = -1;
			} else {
				ctx->active = true;
			}
		}
	}

	if (ctx->active) {
		log_info("Available on SMTP port %d.",
				 SMTP_PORT);
		return 0;
	} else {
		return 1;
	}
}

int start_smtps(smtpscontext * ctx) {
	if (ctx->active) {
		return 0;
	}

	if (sslctx == NULL) {
		log_err("SMTPS: No SSL context available.");
		return 1;
	}
	
	ctx->socket4 = socket(AF_INET,
						  SOCK_STREAM | SOCK_NONBLOCK,
						  0);
	if (ctx->socket4 == -1) {
		log_err("SMTPS: Failed to create IPv4 socket.");
	} else {
		int opt = 1;
		if (setsockopt(ctx->socket4,
					   SOL_SOCKET,
					   SO_REUSEADDR,
					   &opt,
					   sizeof(opt)) != 0) {
			log_warn("SMTPS: Couldn't set an IPv4 socket option.");
		}

		struct sockaddr_in addr4 = {0};
		addr4.sin_family = AF_INET;
		addr4.sin_addr.s_addr = INADDR_ANY;
		addr4.sin_port = htons(SMTPS_PORT);

		if (bind(ctx->socket4,
				 (struct sockaddr *)&addr4,
				 sizeof(addr4)) != 0) {
			log_crit("SMTPS: Unable to bind on the IPv4 socket. %s",
					 strerror(errno));
			close(ctx->socket4);
			ctx->socket4 = -1;
		} else {
			if (listen(ctx->socket4,
					   SOMAXCONN) != 0) {
				log_err("SMTPS: Unable to listen on the IPv4 socket. %s",
						strerror(errno));
				close(ctx->socket4);
				ctx->socket4 = -1;
			} else {
				ctx->active = true;
			}
		}
	}

	ctx->socket6 = socket(AF_INET6,
						  SOCK_STREAM | SOCK_NONBLOCK,
						  0);
	if (ctx->socket6 == -1) {
		log_err("SMTPS: Failed to create IPv6 socket.");
	} else {
		int opt = 1;
		if (setsockopt(ctx->socket6,
					   SOL_SOCKET,
					   SO_REUSEADDR,
					   &opt,
					   sizeof(opt)) != 0) {
			log_warn("SMTPS: Couldn't set an IPv6 socket option.");
		}

		int v6opt = 1;
		if (setsockopt(ctx->socket6,
					   IPPROTO_IPV6,
					   IPV6_V6ONLY,
					   &v6opt,
					   sizeof(v6opt)) != 0) {
			log_warn("SMTPS: Couldn't set an IPv6 socket option.");
		}

		struct sockaddr_in6 addr6 = {0};
		addr6.sin6_family = AF_INET6;
		addr6.sin6_addr = in6addr_any;
		addr6.sin6_port = htons(SMTPS_PORT);

		if (bind(ctx->socket6,
				 (struct sockaddr *)&addr6,
				 sizeof(addr6)) != 0) {
			log_crit("SMTPS: Unable to bind on the IPv6 socket. %s",
					 strerror(errno));
			close(ctx->socket6);
			ctx->socket6 = -1;
		} else {
			if (listen(ctx->socket6,
					   SOMAXCONN) != 0) {
				log_err("SMTPS: Unable to listen on the IPv6 socket. %s",
						strerror(errno));
				close(ctx->socket6);
				ctx->socket6 = -1;
			} else {
				ctx->active = true;
			}
		}
	}

	if (ctx->active) {
		log_info("Available on SMTPS port %d.",
				 SMTPS_PORT);
		return 0;
	} else {
		return 1;
	}
}

int start_starttls(starttlscontext * ctx) {
	if (ctx->active) {
		return 0;
	}

	if (sslctx == NULL) {
		log_err("STARTTLS: No SSL context available.");
		return 1;
	}
	
	ctx->socket4 = socket(AF_INET,
						  SOCK_STREAM | SOCK_NONBLOCK,
						  0);
	if (ctx->socket4 == -1) {
		log_err("STARTTLS: Failed to create IPv4 socket.");
	} else {
		int opt = 1;
		if (setsockopt(ctx->socket4,
					   SOL_SOCKET,
					   SO_REUSEADDR,
					   &opt,
					   sizeof(opt)) != 0) {
			log_warn("STARTTLS: Couldn't set an IPv4 socket option.");
		}

		struct sockaddr_in addr4 = {0};
		addr4.sin_family = AF_INET;
		addr4.sin_addr.s_addr = INADDR_ANY;
		addr4.sin_port = htons(STARTTLS_PORT);

		if (bind(ctx->socket4,
				 (struct sockaddr *)&addr4,
				 sizeof(addr4)) != 0) {
			log_crit("STARTTLS: Unable to bind on the IPv4 socket. %s",
					 strerror(errno));
			close(ctx->socket4);
			ctx->socket4 = -1;
		} else {
			if (listen(ctx->socket4,
					   SOMAXCONN) != 0) {
				log_err("STARTTLS: Unable to listen on the IPv4 socket. %s",
						strerror(errno));
				close(ctx->socket4);
				ctx->socket4 = -1;
			} else {
				ctx->active = true;
			}
		}
	}

	ctx->socket6 = socket(AF_INET6,
						  SOCK_STREAM | SOCK_NONBLOCK,
						  0);
	if (ctx->socket6 == -1) {
		log_err("STARTTLS: Failed to create IPv6 socket.");
	} else {
		int opt = 1;
		if (setsockopt(ctx->socket6,
					   SOL_SOCKET,
					   SO_REUSEADDR,
					   &opt,
					   sizeof(opt)) != 0) {
			log_warn("STARTTLS: Couldn't set an IPv6 socket option.");
		}

		int v6opt = 1;
		if (setsockopt(ctx->socket6,
					   IPPROTO_IPV6,
					   IPV6_V6ONLY,
					   &v6opt,
					   sizeof(v6opt)) != 0) {
			log_warn("STARTTLS: Couldn't set an IPv6 socket option.");
		}

		struct sockaddr_in6 addr6 = {0};
		addr6.sin6_family = AF_INET6;
		addr6.sin6_addr = in6addr_any;
		addr6.sin6_port = htons(STARTTLS_PORT);

		if (bind(ctx->socket6,
				 (struct sockaddr *)&addr6,
				 sizeof(addr6)) != 0) {
			log_crit("STARTTLS: Unable to bind on the IPv6 socket. %s",
					 strerror(errno));
			close(ctx->socket6);
			ctx->socket6 = -1;
		} else {
			if (listen(ctx->socket6,
					   SOMAXCONN) != 0) {
				log_err("STARTTLS: Unable to listen on the IPv6 socket. %s",
						strerror(errno));
				close(ctx->socket6);
				ctx->socket6 = -1;
			} else {
				ctx->active = true;
			}
		}
	}

	if (ctx->active) {
		log_info("Available on StartTLS port %d.",
				 STARTTLS_PORT);
		return 0;
	} else {
		return 1;
	}
}

int start_http(httpcontext * ctx) {
	if (ctx->active) {
		return 0;
	}
	
	ctx->socket4 = socket(AF_INET,
						  SOCK_STREAM | SOCK_NONBLOCK,
						  0);
	if (ctx->socket4 == -1) {
		log_err("HTTP: Failed to create IPv4 socket.");
	} else {
		int opt = 1;
		if (setsockopt(ctx->socket4,
					   SOL_SOCKET,
					   SO_REUSEADDR,
					   &opt,
					   sizeof(opt)) != 0) {
			log_warn("HTTP: Couldn't set an IPv4 socket option.");
		}

		struct sockaddr_in addr4 = {0};
		addr4.sin_family = AF_INET;
		addr4.sin_addr.s_addr = INADDR_ANY;
		addr4.sin_port = htons(HTTP_PORT);

		if (bind(ctx->socket4,
				 (struct sockaddr *)&addr4,
				 sizeof(addr4)) != 0) {
			log_err("HTTP: Unable to bind on the IPv4 socket. %s",
					 strerror(errno));
			close(ctx->socket4);
			ctx->socket4 = -1;
		} else {
			if (listen(ctx->socket4,
					   SOMAXCONN) != 0) {
				log_err("HTTP: Unable to listen on the IPv4 socket. %s",
						strerror(errno));
				close(ctx->socket4);
				ctx->socket4 = -1;
			} else {
				ctx->active = true;
			}
		}
	}

	ctx->socket6 = socket(AF_INET6,
						  SOCK_STREAM | SOCK_NONBLOCK,
						  0);
	if (ctx->socket6 == -1) {
		log_err("HTTP: Failed to create IPv6 socket.");
	} else {
		int opt = 1;
		if (setsockopt(ctx->socket6,
					   SOL_SOCKET,
					   SO_REUSEADDR,
					   &opt,
					   sizeof(opt)) != 0) {
			log_warn("HTTP: Couldn't set an IPv6 socket option.");
		}

		int v6opt = 1;
		if (setsockopt(ctx->socket6,
					   IPPROTO_IPV6,
					   IPV6_V6ONLY,
					   &v6opt,
					   sizeof(v6opt)) != 0) {
			log_warn("HTTP: Couldn't set an IPv6 socket option.");
		}

		struct sockaddr_in6 addr6 = {0};
		addr6.sin6_family = AF_INET6;
		addr6.sin6_addr = in6addr_any;
		addr6.sin6_port = htons(HTTP_PORT);

		if (bind(ctx->socket6,
				 (struct sockaddr *)&addr6,
				 sizeof(addr6)) != 0) {
			log_err("HTTP: Unable to bind on the IPv6 socket. %s",
					 strerror(errno));
			close(ctx->socket6);
			ctx->socket6 = -1;
		} else {
			if (listen(ctx->socket6,
					   SOMAXCONN) != 0) {
				log_err("HTTP: Unable to listen on the IPv6 socket. %s",
						strerror(errno));
				close(ctx->socket6);
				ctx->socket6 = -1;
			} else {
				ctx->active = true;
			}
		}
	}

	if (ctx->active) {
		log_info("Available on HTTP port %d.",
				 HTTP_PORT);
		return 0;
	} else {
		return 1;
	}
}

int start_https(httpscontext * ctx) {
	if (ctx->active) {
		return 0;
	}

	if (sslctx == NULL) {
		log_err("HTTPS: No SSL context available.");
		return 1;
	}
	
	ctx->socket4 = socket(AF_INET,
						  SOCK_STREAM | SOCK_NONBLOCK,
						  0);
	if (ctx->socket4 == -1) {
		log_err("HTTPS: Failed to create IPv4 socket.");
	} else {
		int opt = 1;
		if (setsockopt(ctx->socket4,
					   SOL_SOCKET,
					   SO_REUSEADDR,
					   &opt,
					   sizeof(opt)) != 0) {
			log_warn("HTTPS: Couldn't set an IPv4 socket option.");
		}

		struct sockaddr_in addr4 = {0};
		addr4.sin_family = AF_INET;
		addr4.sin_addr.s_addr = INADDR_ANY;
		addr4.sin_port = htons(HTTPS_PORT);

		if (bind(ctx->socket4,
				 (struct sockaddr *)&addr4,
				 sizeof(addr4)) != 0) {
			log_err("HTTPS: Unable to bind on the IPv4 socket. %s",
					 strerror(errno));
			close(ctx->socket4);
			ctx->socket4 = -1;
		} else {
			if (listen(ctx->socket4,
					   SOMAXCONN) != 0) {
				log_err("HTTPS: Unable to listen on the IPv4 socket. %s",
						strerror(errno));
				close(ctx->socket4);
				ctx->socket4 = -1;
			} else {
				ctx->active = true;
			}
		}
	}

	ctx->socket6 = socket(AF_INET6,
						  SOCK_STREAM | SOCK_NONBLOCK,
						  0);
	if (ctx->socket6 == -1) {
		log_err("HTTPS: Failed to create IPv6 socket.");
	} else {
		int opt = 1;
		if (setsockopt(ctx->socket6,
					   SOL_SOCKET,
					   SO_REUSEADDR,
					   &opt,
					   sizeof(opt)) != 0) {
			log_warn("HTTP: Couldn't set an IPv6 socket option.");
		}

		int v6opt = 1;
		if (setsockopt(ctx->socket6,
					   IPPROTO_IPV6,
					   IPV6_V6ONLY,
					   &v6opt,
					   sizeof(v6opt)) != 0) {
			log_warn("HTTP: Couldn't set an IPv6 socket option.");
		}

		struct sockaddr_in6 addr6 = {0};
		addr6.sin6_family = AF_INET6;
		addr6.sin6_addr = in6addr_any;
		addr6.sin6_port = htons(HTTPS_PORT);

		if (bind(ctx->socket6,
				 (struct sockaddr *)&addr6,
				 sizeof(addr6)) != 0) {
			log_err("HTTPS: Unable to bind on the IPv6 socket. %s",
					 strerror(errno));
			close(ctx->socket6);
			ctx->socket6 = -1;
		} else {
			if (listen(ctx->socket6,
					   SOMAXCONN) != 0) {
				log_err("HTTPS: Unable to listen on the IPv6 socket. %s",
						strerror(errno));
				close(ctx->socket6);
				ctx->socket6 = -1;
			} else {
				ctx->active = true;
			}
		}
	}

	if (ctx->active) {
		log_info("Available on HTTPS port %d.",
				 HTTPS_PORT);
		return 0;
	} else {
		return 1;
	}
}

void check_communications(smtpcontext * smtp,
						 smtpscontext * smtps,
						 starttlscontext * starttls) {
	if (!smtp->active && !smtps->active && !starttls->active) {
		log_crit("CRIT: no communication services were able to be started. aborting...");
		abort();
	}
}

void stop_smtp(smtpcontext * ctx) {
	if (ctx->socket4 != -1) {
		close(ctx->socket4);
		ctx->socket4 = -1;
	}
	if (ctx->socket6 != -1) {
		close(ctx->socket6);
		ctx->socket6 = -1;
	}
	ctx->starttls = false;
	ctx->active = false;
}

void stop_smtps(smtpscontext * ctx) {
	if (ctx->socket4 != -1) {
		close(ctx->socket4);
		ctx->socket4 = -1;
	}
	if (ctx->socket6 != -1) {
		close(ctx->socket6);
		ctx->socket6 = -1;
	}
	ctx->active = false;
}

void stop_starttls(starttlscontext * ctx) {
	if (ctx->socket4 != -1) {
		close(ctx->socket4);
		ctx->socket4 = -1;
	}
	if (ctx->socket6 != -1) {
		close(ctx->socket6);
		ctx->socket6 = -1;
	}
	ctx->active = false;
}

void stop_http(httpcontext * ctx) {
	if (ctx->socket4 != -1) {
		close(ctx->socket4);
		ctx->socket4 = -1;
	}
	if (ctx->socket6 != -1) {
		close(ctx->socket6);
		ctx->socket6 = -1;
	}
	ctx->active = false;
}

void stop_https(httpscontext * ctx) {
	if (ctx->socket4 != -1) {
		close(ctx->socket4);
		ctx->socket4 = -1;
	}
	if (ctx->socket6 != -1) {
		close(ctx->socket6);
		ctx->socket6 = -1;
	}
	ctx->active = false;
}
