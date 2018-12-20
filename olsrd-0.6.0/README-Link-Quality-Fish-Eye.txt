+=====================================================================+
|        Link Quality Fish Eye Mechanism   December 3th 2005          |
|								      |	
|	     	 	    olsrd-0.4.10			      |
+=====================================================================+

	Corinna 'Elektra' Aichele   (onelektra at gmx.net)

---------------
I. Introduction
---------------

Link Quality Fish Eye is a new (experimental) algorithm introduced in
olsrd 0.4.10. To increase stability in a mesh, TC messages should be
sent quite frequently. However, the network would then suffer from the
resulting overhead. The idea is to frequently send TC messages to
adjacent nodes, i.e. nodes that are likely to be involved in routing
loops, without flooding the whole mesh with each sent TC message.

OLSR packets carry a Time To Live (TTL) that specifies the maximal
number of hops that the packets is allowed to travel in the mesh. The
Link Quality Fish Eye mechanism generates TC messages not only with
the default TTL of 255, but with different TTLs, namely 1, 2, 3, and
255, restricting the distribution of TC messages to nodes 1, 2, 3, and
255 hops away. A TC message with a TTL of 1 will just travel to all
one-hop neighbours, a message with a TTL of 2 will in addition reach
all two-hop neighbours, etc.

TC messages with small TTLs are sent more frequently than TC messages
with higher TTLs, such that immediate neighbours are more up to date
with respect to our links than the rest of the mesh. We hope that this
reduces the likelihood of routing loops.


Link Quality Fish Eye是olsrd 0.4.10中引入的一种新的（实验性）算法。为了提高网格的稳定性，应该非常频繁地发送TC消息。
但是，网络将遭受由此产生的开销。该想法是频繁地将TC消息发送到相邻节点，即可能涉及路由环路的节点，而不会使每个发送的TC消息充满整个网格。

OLSR数据包携带生存时间（TTL），指定允许数据包在网格中传播的最大跳数。链路质量鱼眼机制不仅生成TC消息，默认TTL为255，
而且具有不同的TTL，即1,2,3和255，限制TC消息到节点1,2,3和255跳的分布远。 
TTL为1的TC消息将传输到所有一跳邻居，TTL为2的消息将另外到达所有两跳邻居等。

具有较小TTL的TC消息比具有较高TTL的TC消息更频繁地发送，使得直接邻居相对于我们的链路比其余网格更新。
我们希望这可以降低路由循环的可能性。


--------------
II. How to use
--------------

The Fish Eye algorithm can be enabled in the configuration file
/etc/olsrd.conf with the following lines:

	# Fish Eye mechanism for TC messages 0 = off, 1 = on

	LinkQualityFishEye 1

Fish Eye should be used together with a small TcInterval setting as
follows:

	# TC interval in seconds (float)

        TcInterval 0.5

If olsrd is started with debug-level 3 it will print out a message
every time a TC message is issued or dropped.

The following sequence of TTL values is used by olsrd.

        255 3 2 1 2 1 1 3 2 1 2 1 1

Hence, a TC interval of 0.5 seconds leads to the following TC
broadcast scheme.

  * Out of 13 TC messages, all 13 are seen by one-hop neighbours (TTL
    1, 2, 3, or 255), i.e. a one-hop neighbour sees a TC message every
    0.5 seconds.

  * Two-hop neighbours (TTL 2, 3, or 255) see 7 out of 13 TC messages,
    i.e. about one message per 0.9 seconds.

  * Three-hop neighbours (TTL 3 or 255) see 3 out of 13 TC messages,
    i.e. about one message per 2.2 seconds.

  * All other nodes in the mesh (TTL 255) see 1 out of 13 TC messages,
    i.e. one message per 6.5 seconds.

The sequence of TTL values is hardcoded in lq_packet.c and can be
altered easily for further experiments.

The Link Quality Fish Eye algorithm is compatible with earlier
versions of olsrd or nodes that do not have the Fish Eye feature
enabled.

A default configuration file with the Link Quality Fish Eye mechanism
and ETX enabled is located in ./files/olsrd.conf.default.lq-fisheye


可以在配置文件中启用Fish Eye算法
/etc/olsrd.conf包含以下行：
 # Fish Eye mechanism for TC messages 0 = off, 1 = on

	LinkQualityFishEye 1
	
鱼眼应与小TcInterval设置一起使用，如下所示：
# TC interval in seconds (float)

        TcInterval 0.5
	
如果olsrd以调试级别3启动，它将打印出一条消息当每次发出或删除TC消息。
 olsrd使用以下TTL值序列。
 	255 3 2 1 2 1 1 3 2 1 2 1 1
	
因此，0.5秒的TC间隔导致以下TC广播方案。

  *在13个TC消息中，所有13个都被一跳邻居（TTL1,2,3或255）看到，即一跳邻居每0.5秒看到一条TC消息。
  *两跳邻居（TTL 2,3或255）看到13个TC消息中的7个，即每0.9秒约一个消息。
  *三跳邻居（TTL 3或255）看到13条TC消息中的3条，即每2.2秒大约一条消息。
  *网格中的所有其他节点（TTL 255）看到13条TC消息中的1条，即每6.5秒一条消息。

TTL值的序列在lq_packet.c中是硬编码的，并且可以容易地改变以用于进一步的实验。
Link Quality Fish Eye算法与早期版本的olsrd或未启用鱼眼功能的节点兼容。
具有链接质量鱼眼机制和启用ETX的默认配置文件位于./files/olsrd.conf.default.lq-fisheye

---------------
III. Background
---------------

A major problem of a proactive routing algorithm is keeping topology
control information in sync. If topology information is not in sync
routing loops may occur. Usually routing loops happen in a local area
in the mesh - within a few hops.

It may happen that node A assumes that the best way to send packets to
node C is by forwarding them to node B, while node B thinks that the
best route to C is via node A. So A sends the packet to node B, and B
returns it to A - we have a loop. This can of course also happen with
more than two nodes involved in the loop, but it is unlikely that a
routing loop involves more than a few nodes.

Routing information like all data traffic gets lost on radio links
with weak signal-to-noise ratio (SNR) or if collisions occur. Setting
fragmentation and RTS (request-to-send) to conservative values on
wireless interfaces is a must in a mesh to deal with hidden nodes,
interference and collisions. A mesh that utilizes only one channel has
to deal with many collisions between packets and a lot of
self-generated interference. While a radio interface may have a range
of only 300 meters its signals can disturb receivers that are more
than 1000 meters away. The data traffic of a node in the distance is
not readable, but it's signals add to the noise floor in receivers,
reducing signal-to-noise ratio of local links.

When a route is saturated with traffic the transmitters involved
introduce permanent interference into the mesh, causing packet loss on
other links in the neighbourhood. OLSR messages get lost, causing
confusion. While the timeout values of MID messages or HNA messages
can be increased to increase stability without a big tradeoff, TC
messages that are up to date and arrive in time are critical. It is
also critical to have MPR information in sync if the MPR algorithm is
used, but in the author's opinion this optimization doesn't do any
good anyway. The MPR algorithm introduces a new source of failure and
reduces TC message redundancy, so it should be switched off in the
configuration file /etc/olsrd.conf with these lines:

        TcRedundancy 2
        MprCoverage  7

olsrd with LQ Extension attempts to know the best routes all over the
whole mesh cloud, but it is likely that it never will be able to
achieve this in a mesh that has more than a handful of nodes. TC
information is likely to be lost on its way through the whole
mesh. And this likelyhood increases with the number of hops.

But this fact doesn't necessarily harm the routing of traffic on a
long multihop path. A node at one end of a mesh cloud may have the
illusion to know the exact and best path along which its packets
travel when communicating with a node that is several hops away. But
this information may be pretty outdated and incomplete.

In fact all that the algorithm has to achieve is a reasonable choice
for the next two or three hops. If the routing path is 8 hops, for
example, nodes that are 5 hops away from the node initiating traffic
and closer to the destination have better and more accurate
information about the best path. They don't know what a node that
initiates traffic thinks about the path that its packets should take,
they have more accurate routing information and will look into their
routing table and make a choice based on their knowledge.

Someone that sends a snail mail parcel from Europe to India doesn't
have to write the name of the Indian postman on the paket that is
supposed to hand it over to the recipient or the brand of bicycle he
is supposed to ride when transporting the parcel. He doesn't have to
decide the path that the postman has to take in the recipient's
village. The postman knows better than the sender.

It should be sufficient if nodes have a vague idea about the topology
of the mesh in the distance and who is out there. If only a few TC
messages out of many TC packets that are broadcast make it over the
whole mesh this should be sufficient.

主动路由算法的主要问题是保持拓扑控制信息同步。如果拓扑信息不同步，则可能发生循环。通常路由循环发生在网格中的局部区域 - 在几跳内。

可能发生的情况是，节点A假设将数据包发送到节点C的最佳方式是将它们转发到节点B，而节点B认为到C的最佳路由是通过节点A.所以A将数据包发送到节点B，并且B将它返回给A  - 我们有一个循环。当然，这也可能发生在循环中涉及两个以上的节点，但路由循环不太可能涉及多个节点。

所有数据流量的路由信息在无线电链路上丢失
具有较弱的信噪比（SNR）或发生碰撞时。在网状网中设置分段和RTS（请求发送）到保守值是处理隐藏节点，干扰和冲突的必要条件。仅利用一个信道的网格必须处理分组之间的许多冲突和许多自生成的干扰。虽然无线电接口可能只有300米的范围，但其信号可能会干扰距离超过1000米的接收器。距离中节点的数据流量不可读，但它的信号会增加接收机的本底噪声，降低本地链路的信噪比。

当路由饱和流量时，发射机会参与其中
在网格中引入永久性干扰，导致邻域中其他链路丢包。 OLSR消息丢失，导致混淆。虽然可以增加MID消息或HNA消息的超时值以增加稳定性而无需大量权衡，但是最新且及时到达的TC消息是至关重要的。如果使用MPR算法，使MPR信息同步也是至关重要的，但在作者看来，这种优化无论如何都没有任何好处。 MPR算法引入了新的故障源并减少了TC消息冗余，因此应在配置文件/etc/olsrd.conf中使用以下行关闭它：

        TcRedundancy 2
        MprCoverage 7

带有LQ Extension的olsrd试图了解整个网状云的最佳路由，但很可能它永远无法在具有多个节点的网格中实现这一点。 TC信息可能会在整个网格中丢失。而这种可能性随着跳数的增加而增加。

但是这个事实并不一定会损害交通路由长多路径。网状云一端的节点可能具有错觉，以便在与几跳之间的节点通信时知道其数据包沿其行进的确切和最佳路径。但是这些信息可能已经过时且不完整。

实际上，算法必须实现的是接下来的两个或三个跳跃的合理选择。例如，如果路由路径是8跳，则距离发起流量并且更靠近目的地的节点5跳的节点具有关于最佳路径的更好和更准确的信息。他们不知道发起流量的节点对其数据包应该采取的路径的看法是什么，他们有更准确的路由信息​​，并将查看他们的路由表并根据他们的知识做出选择。

从欧洲向印度发送蜗牛邮件包裹的人不必在paket上写下印度邮递员的名字，该邮递员应该将其交给收件人或运输包裹时应该骑的自行车品牌。他不必决定邮递员在收件人村庄必须走的路。邮递员比发件人更清楚。

如果节点对距离中网格的拓扑结构有一个模糊的概念，那么它应该就足够了。如果广播的许多TC分组中只有少数TC消息在整个网格上进行，则这应该足够了
Have fun!

Elektra
