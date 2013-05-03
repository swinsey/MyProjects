package com.pplive.p2p.network
{
	import com.pplive.p2p.network.protocol.Packet;

	public interface ISocketListener
	{
		function onPacket(packet:Packet):void;
		function onSocketLost():void;
	}
}