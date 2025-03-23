const extraCSS = new CSSStyleSheet();
extraCSS.replaceSync(".portB { display: none }");
document.adoptedStyleSheets = [...document.adoptedStyleSheets, extraCSS];
