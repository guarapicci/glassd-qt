

//workspace.clientMaximizeSet.connect(function(client, h, v) {
  //if (h && v) {
    ////print(client.caption + " is fully maximized");
      //callDBus("com.guarapicci.glassd",
                //"/glassd",
                //"com.guarapicci.glassd",
                //"set_application_canonical_name",
                //client.caption,
                //()=>{}
        //);
  //} else {
    //print(client.caption + " is not maximized");
  //}
//});
print("current active client is: " +  workspace.activeClient.caption + "\n");


workspace.clientActivated.connect(function(client) {
  appID=client.desktopFileName;
  //console.log(Object.keys(client));

  print("switched focus to: " +  appID+ "\n");

  callDBus("com.guarapicci.glassd",
          "/glassd",
          "com.guarapicci.glassd",
          "set_application_canonical_name",
          appID,
          function(){
            console.log(Object.keys(ret));
          }
  );

});

// console.log(Object.keys(/*workspace*/))
//workspace.slotSwitchDesktopNext();

//callDBus("com.guarapicci.glassd", "/glassd", "com.guarapicci.glassd", "set_application_canonical_name", "estagios", function() {});

