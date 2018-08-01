/* tweak logo link to the project home page in a new tab*/

$(document).ready(function(){
   $( ".icon-home" ).attr("href", "https://zephyrproject.org/");
   $( ".icon-home" ).attr("target", "_blank");
});
