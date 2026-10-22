#ifndef _LOCALNETPOLICY_H_
#define _LOCALNETPOLICY_H_

#include <QAbstractSocket>
#include <QHostAddress>
#include <QString>

// Offline build ("no cloud"): only addresses that cannot leave this device or
// the local link may be dialled.
//
// Allowed:
//   IPv4  127.0.0.0/8 (loopback), 10/8, 172.16/12, 192.168/16 (RFC1918),
//         169.254/16 (link-local / hotspot without DHCP),
//         0.0.0.0 and 255.255.255.255 (can only ever reach this host)
//   IPv6  ::1 (loopback), fe80::/10 (link-local), fc00::/7 (unique local),
//         :: (the unspecified address)
//
// Everything else - any public address or public name - is refused. The
// check runs on the RESOLVED endpoint, so host names are covered as well.
inline bool IsLocalOrLanHost(const QHostAddress &host)
{
	if (host.isNull())
		return false;

	if (host.protocol() == QAbstractSocket::IPv4Protocol) {
		if (host.isLoopback())
			return true;
		const quint32 v4 = host.toIPv4Address();
		if (v4 == 0u || v4 == 0xffffffffu)
			return true;
		static const struct { const char *net; int prefix; } kV4[] = {
			{ "10.0.0.0",    8  },
			{ "172.16.0.0",  12 },
			{ "192.168.0.0", 16 },
			{ "169.254.0.0", 16 },
		};
		for (const auto &subnet : kV4) {
			if (host.isInSubnet(QHostAddress(QString::fromLatin1(subnet.net)), subnet.prefix))
				return true;
		}
		return false;
	}

	if (host.protocol() == QAbstractSocket::IPv6Protocol) {
		// isInSubnet() warns and returns false on a protocol mismatch, so the
		// family check above is mandatory.
		if (host.isLoopback())
			return true;
		if (host.isInSubnet(QHostAddress(QStringLiteral("fe80::")), 10))
			return true;
		if (host.isInSubnet(QHostAddress(QStringLiteral("fc00::")), 7))
			return true;
		if (host == QHostAddress(QHostAddress::AnyIPv6))
			return true;
		return false;
	}

	return false;
}

#endif // _LOCALNETPOLICY_H_
