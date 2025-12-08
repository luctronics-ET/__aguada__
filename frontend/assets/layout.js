// AGUADA shared layout rendering (header/nav/footer)
(function () {
  const NAV_ITEMS = [
    { href: "index.html", label: "Dashboard" },
    { href: "mapa.html", label: "Mapa" },
    { href: "painel.html", label: "Painel Visual" },
    { href: "dados.html", label: "Dados" },
    { href: "consumo.html", label: "Consumo" },
    { href: "abastecimento.html", label: "Abastecimento" },
    { href: "manutencao.html", label: "Manutenção" },
    { href: "history.html", label: "Histórico" },
    { href: "alerts.html", label: "Alertas" },
    { href: "config.html", label: "Configurações" },
    { href: "system.html", label: "Sistema" },
    { href: "documentacao.html", label: "Docs" },
  ];

  function getActivePage() {
    const path = window.location.pathname.split("/").pop() || "index.html";
    return path.toLowerCase();
  }

  function renderNav(navEl, activePage) {
    if (!navEl) return;
    navEl.classList.add("admin-nav");
    navEl.innerHTML = NAV_ITEMS.map((item) => {
      const isActive = activePage === item.href.toLowerCase();
      return `<a href="${item.href}" class="${isActive ? "active" : ""}">${
        item.label
      }</a>`;
    }).join("");
  }

  function ensureHeader() {
    let header = document.querySelector(".admin-header");

    // If header exists, update the nav and title, but keep structure
    if (header) {
      let titleEl = header.querySelector("h1");
      if (titleEl) {
        titleEl.textContent = document.title || "AGUADA";
      }

      let nav = header.querySelector(".admin-nav");
      if (nav) {
        renderNav(nav, getActivePage());
      }
      return;
    }

    // Create new header if none exists
    header = document.createElement("div");
    header.className = "admin-header";

    let top = document.createElement("div");
    top.className = "admin-header-top";

    let titleEl = document.createElement("h1");
    titleEl.textContent = document.title || "AGUADA";
    top.appendChild(titleEl);

    let status = document.createElement("div");
    status.className = "status-indicator";
    status.innerHTML = '<div class="status-dot"></div><span>● Online</span>';
    top.appendChild(status);

    header.appendChild(top);

    let nav = document.createElement("nav");
    renderNav(nav, getActivePage());
    header.appendChild(nav);

    document.body.prepend(header);
  }

  function ensureFooter() {
    if (document.querySelector(".app-footer")) return;
    const footer = document.createElement("footer");
    footer.className = "app-footer";
    footer.innerHTML = `
      <div class="footer-left">AGUADA • Monitoramento Hidráulico</div>
      <div class="footer-right">Última atualização: <span id="footerTime"></span></div>
    `;
    document.body.appendChild(footer);

    const footerTime = footer.querySelector("#footerTime");
    const update = () => {
      const now = new Date();
      footerTime.textContent = now.toLocaleString("pt-BR");
    };
    update();
    setInterval(update, 1000 * 30);
  }

  document.addEventListener("DOMContentLoaded", () => {
    ensureHeader();
    ensureFooter();
  });
})();
