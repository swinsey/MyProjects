<%@ page contentType="text/html; charset=UTF-8" language="java" pageEncoding="UTF-8" %>
<%@ page import="tool.SuperString" %>
<%
    String url= SuperString.notNullTrim(request.getParameter("url"));
%>
<html>
<head>
<title>PPVA日志系统</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<SCRIPT language=Javascript>if(self!=top) {top.location=self.location;}</SCRIPT>
<link href="css/common.css" rel="stylesheet" type="text/css">
<style type="text/css">
<!--
.STYLE2 {
	color: #0033CC;
	font-weight: bold;
}
.STYLE3 {color: #0033CC}
-->
</style>
</head>
<body bgcolor="#ffffff">
<table width="100%" height="100%"  border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td align="center" valign="top">
	  <p class="font04"><b><br>
	    <br>
      </b></p>
	  <p class="font04">&nbsp;</p>
	  <p class="font04">&nbsp;</p>
	  <p class="font04"><span class="STYLE2">PPVA 日志后台管理系统</span><b><br>
      </b></p>
	  <table width="350"  border="0" cellpadding="20" cellspacing="2" bgcolor="#FF3300">
        <tr>
          <td align="center" bgcolor="#FFFFFF"><table width="300"  border="0" cellspacing="1" cellpadding="2">
            <form name="form1" method="post" action="logindo.jsp">
                <input type="hidden" name="url" value="<%=url%>">
              <tr>
                <td nowrap><span class="STYLE3">请输入管理帐号:</span></td>
                <td nowrap><input name="username" type="text" id="username" value="" size="30" maxlength="20"></td>
              </tr>
              <tr>
                <td width="91" nowrap><span class="STYLE3">请输入管理口令:</span></td>
                <td width="198" nowrap><input name="password" type="password" id="password" value="" size="30" maxlength="20"></td>
              </tr>
              <tr>
                <td nowrap>&nbsp;</td>
                <td nowrap><input name="Submit" type="submit" class="BUTTON2" value=" 进 入 "></td>
              </tr>
            </form>
          </table></td>
        </tr>
      </table>
    <p>&nbsp;</p></td>
  </tr>
</table>
</body>
</html>

